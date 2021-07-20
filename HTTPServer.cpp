// Copyright 2000-2021 Mark H. P. Lord

#include "HTTPServer.h"
#include "ChunkedReader.h"
#include "ChunkedWriter.h"
#include "Clocks.h"
#include "DowngradeLog.h"
#include "GZipFormat.h"
#include "GZipWriter.h"
#include "JSONReader.h"
#include "JSONWriter.h"
#include "MersenneTwister.h"
#include "MultiLog.h"
#include "MultipartParser.h"
#include "PrefixLog.h"
#include "SecureRNG.h"
#include "StreamLoader.h"
#include "StreamLog.h"
#include "StringStream.h"
#include "Substream.h"
#include "TextEncoding.h"
#include "UnclosableStream.h"

namespace Prime {

//
// HTTPServer
//

const char HTTPServer::htmlContentType[] = "text/html";
const char HTTPServer::jsonContentType[] = "application/json";
const char HTTPServer::plainTextContentType[] = "text/plain";
const char HTTPServer::formURLEncodedContentType[] = "application/x-www-form-urlencoded";
const char HTTPServer::multipartFormDataContentType[] = "multipart/form-data";

const char HTTPServer::htmlContentTypeCharsetUTF8[] = "text/html; charset=utf-8";
const char HTTPServer::jsonContentTypeCharsetUTF8[] = "application/json; charset=utf-8";
const char HTTPServer::plainTextContentTypeCharsetUTF8[] = "text/plain; charset=utf-8";

HTTPServer::HTTPServer()
{
}

HTTPServer::~HTTPServer()
{
}

bool HTTPServer::init(Handler* handler, Log* log, Settings* settings)
{
    _handler = handler;

    _log = log;
    _settings = settings;

#ifdef PRIME_CXX11_STL
    _settingsObserver.init(settings, [this](Settings* s) {
        this->updateSettings(s);
    });
#else
    _settingsObserver.init(settings, MethodCallback(this, &HTTPServer::updateSettings));
#endif

    return true;
}

void HTTPServer::updateSettings(Settings* settings)
{
    _verboseLevel = settings->get("verboseLevel").toInt(0);

    _requestOptions.load(settings);
    _responseOptions.load(settings);
}

bool HTTPServer::serve(StreamBuffer* readBuffer, StreamBuffer* writeBuffer, const char* protocol,
    Log* serverLog, PrefixLog* prefixLog, bool canKeepAlive,
    const Value::Dictionary* requestArguments)
{
    StringStream stringStream;
    StreamLog streamLog(&stringStream);

    MultiLog multiLog;
    multiLog.addLog(serverLog);
    multiLog.addLog(&streamLog);

    Log* log = &multiLog;

    Request request;
    request.init(_requestOptions, Clock::getCurrentTime(), log);
    if (requestArguments) {
        request._arguments = *requestArguments;
    }

    Response response;
    response.init(writeBuffer, request.getTime(), _responseOptions);

#ifdef PRIME_CXX11_STLx
    response.setErrorLogCallback([&stringStream]() -> std::string {
        return stringStream.getString();
    });
#else
    class StringStreamStringGetter {
    public:
        StringStreamStringGetter(StringStream& ref)
            : _stringStream(ref)
        {
        }

        std::string getString()
        {
            return _stringStream.getString();
        }

    private:
        StringStream& _stringStream;
    };

    StringStreamStringGetter getter(stringStream);
    response.setErrorLogCallback(MethodCallback(&getter, &StringStreamStringGetter::getString));
#endif

    if (!request.parse(readBuffer, protocol)) {
        if (request.connectionWasClosed()) {
            return false;
        }
        response.setConnectionClose();
        response.errorHTML(400, "Couldn't parse request");
        response.send(log);
        return false;
    }

    response.setRequest(request);

    StringView realIP = request.getRealIP();
    if (!realIP.empty()) {
        prefixLog->setPrefix(MakeString("Client ", realIP));
    }

    // We don't handle Expect headers except 100-continue.
    StringView expect = request.getHeader("expect");
    if (!expect.empty()) {
        if (ASCIIEqualIgnoringCase(StringViewTrim(expect), "100-continue")) {
            // This is handled by Request

        } else {
            response.setConnectionClose();
            response.error(request, 417);
            response.send(log);
            return false;
        }
    }

    // We're responsible for arranging keep-alive.
    response.setKeepAlive(request.isKeepAlive() && canKeepAlive);

#ifdef PRIME_NO_EXCEPTIONS
    if (!_handler->handleRequest(request, response)) {
        response.error(404);
    }
#else
    try {
        if (!_handler->handleRequest(request, response)) {
            response.error(request, 404);
        }
    } catch (Error& error) {
        response.error(request, error.getStatusCode(), error.what());
    }
#endif

    if (!response.send(log) || !writeBuffer->flush(log)) {
        return false;
    }

    bool keepAlive = response.getKeepAlive(); // Value is lost by close(), so remember it

    response.close();

    return keepAlive;
}

//
// HTTPServer::Request::Options
//

HTTPServer::Request::Options::Options()
{
    load(Settings::getNullSettings());
}

bool HTTPServer::Request::Options::load(Settings* settings)
{
    // _maxFormSizeInBytes = settings->get("maxFormSizeInBytes").toUInt(256 * 1024);
    _verboseLevel = settings->get("verboseLevel").toInt(0);
    _multipartFormStreamBufferSize = settings->get("multipartFormStreamBufferSize").toUInt(256 * 1024);
    _multipartMaxHeaderSizeInBytes = settings->get("multipartMaxHeaderSizeInBytes").toUInt(8192);
    _multipartMaxPartSizeInBytes = settings->get("multipartMaxPartSizeInBytes").toUInt(15 * 1024 * 1024);

    return true;
}

//
// HTTPServer::Request
//

bool HTTPServer::Request::parse(StreamBuffer* stream, StringView protocol)
{
    if (!PRIME_GUARD(isInitialised())) {
        return false;
    }

    Log* log = getLog();

    if (!_headers.parse(HTTPParser::ParseModeRequest, stream, true, log)) {
        return false;
    }

    if (!initFromHeaders(protocol)) {
        return false;
    }

    if (_headers.getMethod() == HTTPMethodPost) {
        if (isContentType(formURLEncodedContentType)) {
            if (!parseForm(stream)) {
                return false;
            }

        } else if (isContentType(jsonContentType)) {
            if (!parseJSON(stream)) {
                return false;
            }

        } else if (isContentType(multipartFormDataContentType)) {
            if (!parseMultipartFormData(stream)) {
                return false;
            }

        } else if (getContentLength() == 0) {
            return true;

        } else {
            log->error(MakeString("POST not form encoded: ", _headers["Content-Type"]));
            return false;
        }
    }

    return true;
}

bool HTTPServer::Request::parse(StringView source, StringView protocol)
{
    if (!PRIME_GUARD(isInitialised())) {
        return false;
    }

    Log* log = getLog();

    if (!_headers.parse(HTTPParser::ParseModeRequest, source, true, log)) {
        return false;
    }

    if (isVeryVerboseEnabled()) {
        log->trace(MakeString("Headers: ", _headers.getRawHeaders()));
    }

    return initFromHeaders(protocol);
}

bool HTTPServer::Request::initFromHeaders(StringView protocol)
{
    if (!PRIME_GUARD(isInitialised())) {
        return false;
    }

    Log* log = getLog();

    _url = _headers.getAbsoluteURL(protocol);

    if (isVerboseEnabled()) {
        log->verbose("%s %s HTTP/%d.%d", GetHTTPMethodName(_headers.getMethod()), _url.toString().c_str(),
            _headers.getMajorVersion(), _headers.getMinorVersion());
    }

    if (isVeryVerboseEnabled()) {
        log->trace(MakeString("Headers: ", _headers.getRawHeaders()));
    }

    _path.parse(_url.getPath());

    _acceptJSON = _acceptHTML = 0;

    HTTPQValueParser qvp(_headers.get("Accept"));
    HTTPQValueParser::QValue q;
    while (qvp.read(q)) {
        if (ASCIIEqualIgnoringCase(q.name, jsonContentType)) {
            _acceptJSON = q.q;
        }
        if (ASCIIEqualIgnoringCase(q.name, htmlContentType)) {
            _acceptHTML = q.q;
        }
    }

    if (ASCIIEqualIgnoringCase(StringViewTrim(_headers["Expect"]), "100-continue")) {
        _expect100 = true;
    } else {
        _expect100 = false;
    }

    _pathOffset = 0;

    return true;
}

std::vector<std::string> HTTPServer::Request::getFormStringVector(StringView name) const
{
    return ToStringVector(getForm(name), "");
}

Optional<int64_t> HTTPServer::Request::getContentLength() const
{
    int64_t length;
    if (StringToInt(_headers["Content-Length"], length) && length >= 0) {
        return length;
    }

    return nullopt;
}

RefPtr<Stream> HTTPServer::Request::getStream(StreamBuffer* rawStream, Log* log)
{
    // TODO: respect options.maxFormSizeInBytes
    if (_expect100) {
        PRIME_ASSERT(rawStream->getBytesAvailable() == 0);
        if (!rawStream->flush(log) || !rawStream->printf(log, "HTTP/1.1 100 continue\r\n\r\n") || !rawStream->flush(log)) {
            return NULL;
        }

        _expect100 = false;
    }

    RefPtr<Stream> stream = static_cast<Stream*>(rawStream);
    StringView transferEncoding = _headers["Transfer-Encoding"];
    if (ASCIIEqualIgnoringCase(transferEncoding, "chunked")) {
        return PassRef(new ChunkedReader(rawStream));
    }

    if (Optional<int64_t> length = getContentLength()) {
        return PassRef(new Substream(stream, 0, false, *length, log, false));
    }

    log->error(PRIME_LOCALISE("Unable to read POST."));
    return NULL;
}

bool HTTPServer::Request::parseForm(StreamBuffer* rawStream)
{
    RefPtr<Stream> postStream = getStream(rawStream, _log);
    if (!postStream) {
        return false;
    }

    StreamLoader post;
    if (!post.load(postStream, _log)) {
        return false;
    }

    URL::parseQueryString(_json.resetDictionary(), post.getString());

    if (isVerboseEnabled()) {
        _log->trace(MakeString("Form: ", post.getString()));
    }

    return true;
}

bool HTTPServer::Request::parseMultipartFormData(StreamBuffer* rawStream)
{
    RefPtr<Stream> rawStreamWrapper = getStream(rawStream, _log);
    if (!rawStreamWrapper) {
        return false;
    }

    MultipartParser multipart;
    if (!multipart.init(rawStreamWrapper,
            MultipartParser::parseBoundary(getHeader("Content-Type")),
            _options._multipartFormStreamBufferSize,
            _log)) {
        return false;
    }

    Value::Dictionary& dictionary = _json.resetDictionary();

    while (RefPtr<Stream> stream = multipart.readPart(_log)) {
        StreamBuffer buffer(stream, _options._multipartMaxHeaderSizeInBytes);

        HTTPParser http;
        if (!http.parse(HTTPParser::ParseModeHeadersOnly, &buffer, false, _log)) {
            return false;
        }

        //_log->trace(MakeString("Headers: ", http.getRawHeaders())); // Requires copy=true in above arguments to http.parse

        StringView dispositionHeader = http.get("Content-Disposition");
        if (dispositionHeader.empty()) {
            _log->error(PRIME_LOCALISE("multipart part is missing Content-Disposition"));
            return false;
        }

        StringView disposition = HTTPParseToken(dispositionHeader, dispositionHeader);
        if (!ASCIIEqualIgnoringCase(disposition, "form-data")) {
            _log->error(PRIME_LOCALISE("multipart part is not form-data"));
            return false;
        }

        std::string fieldName;

        for (;;) {
            if (!HTTPSkip(dispositionHeader, ";", dispositionHeader)) {
                break;
            }

            StringView name = HTTPParseToken(dispositionHeader, dispositionHeader);
            if (HTTPSkip(dispositionHeader, "=", dispositionHeader)) {
                std::string value = HTTPParseTokenOrQuotedString(dispositionHeader, dispositionHeader);

                if (name == "name") {
                    fieldName.swap(value);
                } else if (name == "filename") {
                    dictionary.set(fieldName + "__filename", value);
                }
            }
        }

        if (fieldName.empty()) {
            _log->error(PRIME_LOCALISE("multipart part has no name"));
            return false;
        }

        std::string fieldValue;

        for (;;) {
            char buf[256];
            ptrdiff_t nread = buffer.readSome(&buf, sizeof(buf), _log);
            if (nread < 0) {
                return false;
            }
            if (nread == 0) {
                break;
            }

            fieldValue.insert(fieldValue.end(), buf, buf + nread);
            if (fieldValue.size() > _options._multipartMaxPartSizeInBytes) {
                _log->error(PRIME_LOCALISE("Upload exceeds maximum (%" PRIuPTR ")"), _options._multipartMaxPartSizeInBytes);
                return false;
            }
        }

        dictionary.set(fieldName, fieldValue);
    }

    if (!multipart.atEnd()) {
        return false;
    }

    return true;
}

bool HTTPServer::Request::isAJAXRequest() const
{
    return ASCIIEqualIgnoringCase(getHeader("X-Requested-With"), "xmlhttprequest");
}

bool HTTPServer::Request::isContentType(StringView contentType) const
{
    return ASCIIEqualIgnoringCase(StringViewTrim(StringViewBisect(getHeader("Content-Type"), ';').first), contentType);
}

bool HTTPServer::Request::parseJSON(StreamBuffer* rawStream)
{
    RefPtr<Stream> postStream = getStream(rawStream, _log);
    if (!postStream) {
        return false;
    }

    JSONReader jsonReader;
    _json = jsonReader.read(postStream, _log);

    if (isVerboseEnabled()) {
        _log->trace(MakeString("JSON: ", ToJSON(_json)));
    }

    return !_json.isUndefined();
}

StringView HTTPServer::Request::getRealIP() const
{
    StringView forwardedFor = getHeader("X-Forwarded-For");
    if (!forwardedFor.empty()) {
        return forwardedFor;
    }

    StringView realIP = getHeader("X-Real-IP");
    if (!realIP.empty()) {
        return realIP;
    }

    return StringView();
}

std::string HTTPServer::Request::getQueryString(StringView name) const
{
    return _url.getQuery(name);
}

Optional<std::string> HTTPServer::Request::getOptionalQueryString(StringView name) const
{
    auto array = _url.getQueryArray(name);
    if (array.empty()) {
        return nullopt;
    }

    return array[0];
}

std::string HTTPServer::Request::getParameterString(StringView name) const
{
    return _url.getParameter(name);
}

std::vector<std::string> HTTPServer::Request::getQueryStringVector(StringView name) const
{
    return _url.getQueryArray(name);
}

Value::Dictionary HTTPServer::Request::getQueriesDictionary() const
{
    Value::Dictionary dictionary;

    URLQueryParser qs(_url.getQuery());
    URLQueryParser::Parameter qsp;
    while (qs.read(qsp)) {
        dictionary.set(URLDecode(qsp.name, URLDecodeFlagPlusesAsSpaces), URLDecode(qsp.value, URLDecodeFlagPlusesAsSpaces));
    }

    return dictionary;
}

StringView HTTPServer::Request::getEncodedCookie(StringView name) const
{
    return _headers.getEncodedCookie(name);
}

std::string HTTPServer::Request::getCookie(StringView name) const
{
    return _headers.getCookie(name);
}

double HTTPServer::Request::getAcceptEncoding(StringView name) const
{
    return HTTPQValueParser::getQValue(getHeader("Accept-Encoding"), name);
}

double HTTPServer::Request::getAccept(StringView name) const
{
    return HTTPQValueParser::getQValue(getHeader("Accept"), name);
}

void HTTPServer::Request::mergeArguments(Value::Dictionary arguments)
{
    for (size_t i = 0; i != arguments.size(); ++i) {
        const Value::Dictionary::value_type& pair = arguments.pair(i);
        _arguments.set(PRIME_MOVE(pair.first), PRIME_MOVE(pair.second));
    }
}

URLPath HTTPServer::Request::getRemainingPath() const
{
    return getPath().getTail(getPathOffset());
}

std::string HTTPServer::Request::getRemainingPathString() const
{
    return getRemainingPath().toString(URLPath::StringOptions().setSkipUnsafeComponents().setWithoutLeadingSlash().setWithoutEscaping());
}

void HTTPServer::Request::setRerouteCallback(RerouteCallback callback)
{
    _rerouteCallback = callback;
}

bool HTTPServer::Request::canReroute() const
{
    return !!_rerouteCallback;
}

bool HTTPServer::Request::reroute(const URLPath& path, Request& request, Response& response)
{
    if (!PRIME_GUARD(canReroute())) {
        return false;
    }

    return _rerouteCallback(path, request, response);
}

//
// HTTPServer::Response::Options
//

HTTPServer::Response::Options::Options()
{
    load(Settings::getNullSettings());
}

bool HTTPServer::Response::Options::load(Settings* settings)
{
    _responseBufferSize = settings->get("responseBufferSize").toUInt(64 * 1024);
    _useZeroCopy = settings->get("useZeroCopy").toBool(true);
    _gzipDynamicContentSizeInBytes = settings->get("gzipDynamicContentSizeInBytes").toInt(1024);
    _gzipStaticContentSizeInBytes = settings->get("gzipStaticContentSizeInBytes").toInt(1024);
    _gzipCompressInMemorySizeInBytes = settings->get("gzipCompressInMemorySizeInBytes").toInt(128 * 1024);
    _gzipChunked = settings->get("gzipChunked").toBool(true);
    _gzipCompressionLevel = settings->get("gzipCompressionLevel").toInt(4);
    _verboseLevel = settings->get("verboseLevel").toInt(0);

    return true;
}

//
// HTTPServer::Response
//

namespace {
    struct ResponseCodeAndDescription {
        int responseCode;
        const char* description;

        bool operator<(const ResponseCodeAndDescription& rhs) const
        {
            return responseCode < rhs.responseCode;
        }
    };
}

const char* HTTPServer::Response::descriptionForHTTPResponseCode(int responseCode)
{
    static const ResponseCodeAndDescription list[] = {
        { 200, "OK" },
        { 201, "Created" },
        { 202, "Accepted" },
        { 203, "Non-Authoritative Information" },
        { 204, "No Content" },
        { 205, "Reset Content" },
        { 206, "Partial Content" },
        { 207, "Multi-Status" },
        { 208, "Already Reported" },
        { 226, "IM Used" },
        { 300, "Multiple Choices" },
        { 301, "Moved Permanently" },
        { 302, "Found" },
        { 303, "See Other" },
        { 304, "Not Modified" },
        { 305, "Use Proxy" },
        { 306, "Switch Proxy" },
        { 307, "Temporary Redirect" },
        { 308, "Permanent Redirect" },
        { 400, "Bad Request" },
        { 401, "Unauthorized" },
        { 402, "Payment Required" },
        { 403, "Forbidden" },
        { 404, "Not Found" },
        { 405, "Method Not Allowed" },
        { 406, "Not Acceptable" },
        { 407, "Proxy Authentication Required" },
        { 408, "Request Timeout" },
        { 409, "Conflict" },
        { 410, "Gone" },
        { 411, "Length Required" },
        { 412, "Precondition Failed" },
        { 413, "Request Entity Too Large" },
        { 414, "Request-URI Too Long" },
        { 415, "Unsupported Media Type" },
        { 416, "Requested Range Not Satisfiable" },
        { 417, "Expectation Failed" },
        { 418, "I'm a teapot" },
        { 420, "Enhance Your Calm" },
        { 422, "Unprocessable Entity" },
        { 423, "Locked" },
        { 424, "Failed Dependency" },
        { 424, "Method Failure" },
        { 425, "Unordered Collection" },
        { 426, "Upgrade Required" },
        { 428, "Precondition Required" },
        { 429, "Too Many Requests" },
        { 431, "Request Header Fields Too Large" },
        { 432, "Request Thrashed" },
        { 444, "No Response" },
        { 449, "Retry With" },
        { 450, "Blocked by Windows Parental Controls" },
        { 451, "Unavailable For Legal Reasons" },
        { 451, "Redirect" },
        { 494, "Request Header Too Large" },
        { 495, "Cert Error" },
        { 496, "No Cert" },
        { 497, "HTTP to HTTPS" },
        { 499, "Client Closed Request" },
        { 500, "Internal Server Error" },
        { 501, "Not Implemented" },
        { 502, "Bad Gateway" },
        { 503, "Service Unavailable" },
        { 504, "Gateway Timeout" },
        { 505, "HTTP Version Not Supported" },
        { 506, "Variant Also Negotiates" }, // RFC 2295
        { 507, "Insufficient Storage" }, // WebDAV; RFC 4918
        { 508, "Loop Detected" }, // WebDAV; RFC 5842
        { 509, "Bandwidth Limit Exceeded" }, // Apache bw/limited extension
        { 510, "Not Extended" }, // RFC 2774
        { 511, "Network Authentication Required" }, // RFC 6585
        { 598, "Network read timeout error" }, // Unknown
        { 599, "Network connect timeout error" }, // Unknown
    };

    ResponseCodeAndDescription needle = { responseCode, "" };
    const ResponseCodeAndDescription* match = std::lower_bound(list, list + PRIME_COUNTOF(list), needle);

    if (match == list + PRIME_COUNTOF(list) || match->responseCode != responseCode) {
        return "Unknown HTTP response code";
    }

    return match->description;
}

void HTTPServer::Response::construct()
{
    _sent = false;
    _keepAlive = false;
    _headerOnly = false;
    _method = HTTPMethodUnknown;
    setResponseCode(200);
    _acceptGZip = false;
}

HTTPServer::Response::Response()
{
    construct();
}

HTTPServer::Response::~Response()
{
}

void HTTPServer::Response::init(StreamBuffer* stream, const UnixTime& time, const Options& options)
{
    _stream = stream;
    _time = time;
    _options = options;

    _sent = false;
    _keepAlive = false;

    setHeader("Server", "Prime/1.0");
    setHeader("Date", time);
    setHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    setHeader("Expires", "0");
}

void HTTPServer::Response::setRequest(Request& request)
{
    _method = request.getMethod();
    _url = request.getURL();
    _headerOnly = (_method == HTTPMethodHead);

    _acceptGZip = request.getAcceptEncoding("gzip") > 0;
}

void HTTPServer::Response::close()
{
    _stream.release();
    _content.resize(0);
    construct();
}

void HTTPServer::Response::setResponseCode(int responseCode)
{
    setResponseCode(responseCode, descriptionForHTTPResponseCode(responseCode));
}

void HTTPServer::Response::setResponseCode(int responseCode, StringView responseCodeText)
{
    PRIME_ASSERT(!_sent);
    _headers.setResponseCode(responseCode);
    _headers.setResponseCodeText(responseCodeText);
}

void HTTPServer::Response::error(Request& request, int responseCode, StringView what)
{
    setResponseCode(responseCode);
    if (request.wantsJSON()) {
        Value::Dictionary dict;
        if (what.empty()) {
            dict.set("error", _headers.getResponseCodeText());
        } else {
            dict.set("error", what);
        }
        setJSON(dict);
        return;
    }

    errorHTML(responseCode, what);
}

void HTTPServer::Response::errorHTML(int responseCode, StringView what)
{
    setResponseCode(responseCode);
    std::string log;
    if (_errorLogCallback) {
        log = _errorLogCallback();
    }
    setHTMLBody(MakeString("<h1>Error ", responseCode, ": ", _headers.getResponseCodeText(), "</h1>"
                                                                                             "<p>",
        HTMLEscape(what), "</p>"
                          "<p><pre>",
        log, "</pre></p>"));
}

void HTTPServer::Response::setErrorLogCallback(const ErrorLogCallback& callback)
{
    _errorLogCallback = callback;
}

void HTTPServer::Response::setHeader(StringView name, StringView value)
{
    PRIME_ASSERT(!_sent);
    _headers.set(name, value);
}

void HTTPServer::Response::addHeader(StringView name, StringView value)
{
    PRIME_ASSERT(!_sent);
    _headers.add(name, value);
}

void HTTPServer::Response::removeHeader(StringView name)
{
    PRIME_ASSERT(!_sent);
    _headers.remove(name);
}

void HTTPServer::Response::setKeepAlive(bool value)
{
    if (value) {
        setConnectionKeepAlive();
    } else {
        setConnectionClose();
    }
}

void HTTPServer::Response::setConnectionClose()
{
    setHeader("Connection", "close");
    _keepAlive = false;
}

void HTTPServer::Response::setConnectionKeepAlive()
{
    setHeader("Connection", "keep-alive");
    _keepAlive = true;
}

void HTTPServer::Response::setHTML(std::string html)
{
    PRIME_ASSERT(!_sent);
    _content.swap(html);
    setContentLength(_content.size());
    setContentType(htmlContentTypeCharsetUTF8);
}

void HTTPServer::Response::setHTMLBody(std::string html)
{
    html.insert(0, "<!DOCTYPE html><html><body>");
    html.append("</body></html>\r\n");
    _content.swap(html);
    setContentLength(_content.size());
    setContentType(htmlContentTypeCharsetUTF8);
}

void HTTPServer::Response::setContent(std::string content, StringView contentType)
{
    PRIME_ASSERT(!_sent);
    _content.swap(content);
    setContentLength(_content.size());
    setContentType(contentType);
}

void HTTPServer::Response::appendContent(StringView string)
{
    _content.append(string.begin(), string.end());
    setContentLength(_content.size()); // TODO: can we defer this until the end?
}

void HTTPServer::Response::clearContent()
{
    PRIME_ASSERT(!_sent);
    _content.resize(0);
}

void HTTPServer::Response::setContentType(StringView type)
{
    setHeader("Content-Type", type);
}

void HTTPServer::Response::setContentLength(uint64_t length)
{
    setHeader("Content-Length", ToString(length));
}

bool HTTPServer::Response::send(Log* log)
{
    if (_sent) {
        return true;
    }

    if (getHeader("Expires") == "0") {
        addHeader("Pragma", "no-cache");
    }

    if (_method == HTTPMethodUnknown) {
        log->verbose("%d invalid request", _headers.getResponseCode());
    } else {
        // Report processing time (i.e., don't report the send() time)
        Log::Level level = _headers.getResponseCode() >= 400 ? Log::LevelError : Log::LevelVerbose;
        log->log(level,
            "%-3d %7.2lfms %-4s %s",
            _headers.getResponseCode(),
            (Clock::getCurrentTime().toDouble() - _time.toDouble()) * 1000,
            GetHTTPMethodName(_method),
            _url.toString(URL::StringOptions() /*.setLogSafe()*/).c_str());
    }

    if (!_stream) {
        return false;
    }

#ifndef PRIME_NO_ZLIB

    StringStream gziped;
    bool isGZiped = false;

    if (shouldGZip() && _content.size() >= (size_t)_options._gzipDynamicContentSizeInBytes) {
        if (getHeader("Content-Encoding").empty()) {
            PrefixLog gzipPrefixLog(log, "gzip");
            DowngradeLog gzipLog(&gzipPrefixLog, Log::LevelWarning);

            GZipWriter gziper;

            if (!gziper.begin(&gziped, _options._gzipCompressionLevel, log) || !gziper.writeExact(_content.data(), _content.size(), &gzipLog) || !gziper.end(&gzipLog)) {

                // It's OK - we'll send it uncompressed.
            } else {
                setContentLength(gziped.getSize());
                isGZiped = true;
                setHeader("Content-Encoding", "gzip");

                if (_options._verboseLevel >= 2) {
                    log->trace("Compressed dynamic content from %" PRIuPTR " bytes to %" PRIuPTR " bytes.",
                        _content.size(), gziped.getSize());
                }
            }
        }
    }

#endif

    _sent = true;

    if (_options._verboseLevel >= 2) {
        log->trace(MakeString("Response headers: ", _headers.getRawHeaders()));
    }

    if (!_headers.send(_stream, log)) {
        return false;
    }

#ifndef PRIME_NO_ZLIB

    if (isGZiped) {
        if (_options._verboseLevel >= 2) {
            log->trace("Writing gziped (%" PRIME_PRId_STREAM " bytes).", gziped.getSize());
        }
        if (!_stream->writeExact(gziped.getBytes(), gziped.getSize(), log)) {
            return false;
        }
    } else

#endif
        /* else */ if (!_content.empty()) {
        if (_options._verboseLevel >= 2) {
            log->trace("Writing raw (%" PRIuPTR " bytes).", (size_t)_content.size());
        }
        if (!_stream->writeExact(_content.data(), _content.size(), log)) {
            return false;
        }
    }

    return true;
}

void HTTPServer::Response::redirect(StringView path)
{
    PRIME_ASSERT(!_sent);
    setHeader("Location", path);
    setResponseCode(302, "Found");
    setHTMLBody(MakeString("<p>Please <a href=\"", HTMLEscape(path), "\">click here</a>.</p>"));
}

void HTTPServer::Response::setJSON(const Value::Dictionary& dictionary)
{
    setContent(ToJSON(dictionary), jsonContentTypeCharsetUTF8);
}

void HTTPServer::Response::setPlainText(std::string text)
{
    _content.swap(text);
    setContentLength(_content.size());
    setContentType(plainTextContentTypeCharsetUTF8);
}

bool HTTPServer::Response::sendStream(Stream* stream, Log* log, const SendStreamOptions& sendOptions)
{
    PRIME_ASSERT(_content.empty());

    Stream::Offset size = stream->getSize(Log::getNullLog());

    return (size < 0) ? sendStreamChunked(stream, log, sendOptions) : sendStream(stream, size, log, sendOptions);
}

bool HTTPServer::Response::sendStream(Stream* stream, Stream::Offset size, Log* log,
    const SendStreamOptions& sendOptions)
{
    PRIME_ASSERT(_content.empty());
    PRIME_ASSERT(size >= 0);

#ifndef PRIME_NO_ZLIB

    if (shouldGZip() && !sendOptions.isAlreadyCompressed() && _options._gzipStaticContentSizeInBytes >= 0 && size >= _options._gzipStaticContentSizeInBytes) {

        return compressAndSendStream(stream, size, log, sendOptions);
    }

#endif

    if (sendOptions.isRawDeflated()) {
        // We will be adding a gzip header and footer as some browsers mess up "Content-Encoding: deflate".
        setContentLength((uint64_t)size + GZip::Header::defaultEncodedSize + GZip::Footer::encodedSize);
        setHeader("Content-Encoding", "gzip");

    } else {
        setContentLength((uint64_t)size);
    }

    if (!send(log)) {
        return false;
    }

    if (isHeaderOnly()) {
        return true;
    }

    if (sendOptions.isRawDeflated()) {
        if (_options._verboseLevel >= 2) {
            log->trace("Sending gzip header.");
        }
        if (!sendGZipHeader(_stream, log)) {
            return false;
        }
    }

    if (_options._useZeroCopy) {
        if (_options._verboseLevel >= 2) {
            log->trace("Sending %" PRIME_PRId_STREAM " bytes from a Stream (trying zero-copy).", size);
        }

        if (!_stream->flushWrites(log)) {
            return false;
        }

        // Send the source Stream directly to the underlying socket, allowing zero-copy to be used where available.

        if (!_stream->getUnderlyingStream()->copyFrom(stream, log, size, log, _options._responseBufferSize)) {
            return false;
        }
    } else {

        // Send via _stream's buffer, which precludes zero-copy being used.

        if (_options._verboseLevel >= 2) {
            log->trace("Sending %" PRIME_PRId_STREAM " bytes from a Stream.", size);
        }

        if (!_stream->copyFrom(stream, log, size, log, _options._responseBufferSize)) {
            return false;
        }
    }

    if (sendOptions.isRawDeflated()) {
        if (_options._verboseLevel >= 2) {
            log->trace("Sending gzip footer.");
        }

        if (!sendGZipFooter(_stream, (uint32_t)size, sendOptions.getCRC32(), log)) {
            return false;
        }
    }

    return true;
}

bool HTTPServer::Response::sendGZipHeader(Stream* stream, Log* log)
{
    GZip::Header header;
    char headerBytes[GZip::Header::defaultEncodedSize];
    header.encode(headerBytes);
    return stream->writeExact(headerBytes, sizeof(headerBytes), log);
}

bool HTTPServer::Response::sendGZipFooter(Stream* stream, uint32_t originalSize, uint32_t crc32, Log* log)
{
    GZip::Footer footer;
    footer.originalSize = (uint32_t)originalSize;
    footer.crc32 = crc32;
    char footerBytes[GZip::Footer::encodedSize];
    return stream->writeExact(footerBytes, sizeof(footerBytes), log);
}

bool HTTPServer::Response::sendStreamChunked(Stream* stream, Log* log, const SendStreamOptions& sendOptions)
{
    PRIME_ASSERT(_content.empty());

    // Can't send raw deflated because we need to know a CRC-32, and if you know that then you have to know the
    // size.
    PRIME_ASSERT(!sendOptions.isRawDeflated());

#ifndef PRIME_NO_ZLIB

    if (!sendOptions.isAlreadyCompressed() && _options._gzipChunked && shouldGZip()) {
        return compressAndSendStreamChunked(stream, log, sendOptions);
    }

#endif

    setHeader("Transfer-Encoding", "chunked");

    if (!send(log)) {
        return false;
    }

    if (isHeaderOnly()) {
        return true;
    }

    if (_options._verboseLevel >= 2) {
        log->trace("Sending Stream chunked.");
    }

    ChunkedWriter chunker(_stream);

    if (!chunker.copyFrom(stream, log, -1, log, _options._responseBufferSize)) {
        return false;
    }

    if (!chunker.end(log)) {
        return false;
    }

    return true;
}

RefPtr<Stream> HTTPServer::Response::beginStream(uint64_t contentLength, Log* log)
{
    setContentLength((uint64_t)contentLength);

    if (!send(log)) {
        return NULL;
    }

    if (!isHeaderOnly()) {
        return NULL;
    }

    if (_options._verboseLevel >= 2) {
        log->trace("Returning Stream to application to write response (raw).");
    }

    return PassRef(new UnclosableStream(_stream));
}

RefPtr<Stream> HTTPServer::Response::beginChunked(Log* log, const SendStreamOptions& options)
{
#ifndef PRIME_NO_ZLIB

    bool gzip = shouldGZip() && !options.isAlreadyCompressed();

    if (gzip) {
        setHeader("Content-Encoding", "gzip");
    }

    setHeader("Transfer-Encoding", "chunked");

    if (!send(log)) {
        return NULL;
    }

    if (isHeaderOnly()) {
        return NULL;
    }

    RefPtr<ChunkedWriter> chunkedStream = PassRef(new ChunkedWriter(PassRef(new UnclosableStream(_stream))));

    if (!gzip) {
        return chunkedStream;
    }

    RefPtr<GZipWriter> gzipStream = PassRef(new GZipWriter);
    if (!gzipStream->begin(chunkedStream, _options._gzipCompressionLevel, log)) {
        errorHTML(500, "Couldn't initialise gzip stream");
        return NULL;
    }

    if (_options._verboseLevel >= 2) {
        log->trace("Returning Stream to application to write response (gzip'd).");
    }

    return gzipStream;

#else

    (void)options;

    setHeader("Transfer-Encoding", "chunked");

    if (!send(log)) {
        return NULL;
    }

    if (isHeaderOnly()) {
        return NULL;
    }

    RefPtr<ChunkedWriter> chunkedStream = PassRef(new ChunkedWriter(PassRef(new UnclosableStream(_stream))));
    return chunkedStream;

#endif
}

#ifndef PRIME_NO_ZLIB

bool HTTPServer::Response::compressAndSendStream(Stream* stream, Stream::Offset size, Log* log, const SendStreamOptions& sendOptions)
{
    PRIME_ASSERT(_content.empty());
    PRIME_ASSERT(!sendOptions.isRawDeflated());
    PRIME_ASSERT(!sendOptions.isAlreadyCompressed());

    if (size > _options._gzipCompressInMemorySizeInBytes) {
        return compressAndSendStreamChunked(stream, log, sendOptions);
    }

    StringStream memory;
    Stream::Offset originalSize;
    if (!gzip(&memory, stream, log, &originalSize)) {
        return false;
    }

    memory.setOffset(0, log);

    if (_options._verboseLevel >= 2) {
        log->trace("In-memory compressed static content from %" PRIME_PRId_STREAM " bytes to %" PRIuPTR " bytes.", originalSize, memory.getSize());
    }

    SendStreamOptions newSendOptions(sendOptions);
    newSendOptions.setAlreadyCompressed(true);
    newSendOptions.setRawDeflated(false);

    setHeader("Content-Encoding", "gzip");

    return sendStream(&memory, (Stream::Offset)memory.getSize(), log, newSendOptions);
}

bool HTTPServer::Response::compressAndSendStreamChunked(Stream* stream, Log* log, const SendStreamOptions&)
{
    PRIME_ASSERT(_content.empty());

    setHeader("Transfer-Encoding", "chunked");
    setHeader("Content-Encoding", "gzip");

    if (!send(log)) {
        return false;
    }

    if (isHeaderOnly()) {
        return true;
    }

    // stream -> crcStream -> deflater -> chunker -> _stream

    ChunkedWriter chunker(_stream);

    if (_options._verboseLevel >= 2) {
        log->trace("Sending Stream through gzip through chunker.");
    }

    Stream::Offset originalSize;
    if (!gzip(&chunker, stream, log, &originalSize)) {
        return false;
    }

    if (!chunker.end(log)) {
        return false;
    }

    if (_options._verboseLevel >= 2) {
        log->trace("Compressed static content from %" PRIME_PRId_STREAM " bytes to %" PRIuPTR " bytes.", originalSize, chunker.getBytesWritten());
    }

    return true;
}

bool HTTPServer::Response::gzip(Stream* out, Stream* in, Log* log, Stream::Offset* originalSizeOut)
{
    GZipWriter gziper;
    gziper.begin(out, _options._gzipCompressionLevel, log);

    if (!gziper.copyFrom(in, log, -1, log, _options._responseBufferSize)) {
        return false;
    }

    if (!gziper.end(log)) {
        return false;
    }

    if (originalSizeOut) {
        *originalSizeOut = (Stream::Offset)gziper.getBytesWritten();
    }

    return true;
}

#endif // PRIME_NO_ZLIB

void HTTPServer::Response::setHeader(StringView name, const UnixTime& unixTime)
{
    PRIME_ASSERT(!_sent);
    _headers.set(name, unixTime);
}

void HTTPServer::Response::setExpirationSeconds(int expireAfterSeconds)
{
    UnixTime expirationTime = _time + UnixTime(expireAfterSeconds);

    setHeader("Expires", expirationTime);

    setHeader("Cache-Control", MakeString("max-age=", expireAfterSeconds));

    if (!hasHeader("Last-Modified")) {
        setHeader("Last-Modified", _time);
    }
}

void HTTPServer::Response::setCookie(StringView cookie)
{
    addHeader("Set-Cookie", cookie);
}

void HTTPServer::Response::setCookie(StringView name, StringView value, const UnixTime& expire, StringView path)
{
    char expiresRFC1123[64];
    DateTime(expire).toRFC1123(expiresRFC1123, sizeof(expiresRFC1123));
    setCookie(MakeString(name, '=', URLEncode(value), "; Path=", path, "; HTTPOnly; expires=", expiresRFC1123));
}

void HTTPServer::Response::setSessionCookie(StringView name, StringView value, StringView path)
{
    setCookie(MakeString(name, '=', URLEncode(value), "Path=", path, "; HTTPOnly"));
}

void HTTPServer::Response::deleteCookie(StringView name, StringView path)
{
    setCookie(MakeString(name, "=; Path=", path, "; HTTPOnly; expires=Thu, 01 Jan 1970 00:00:00 GMT"));
}

//
// HTTPServer::Session
//

PRIME_DEFINE_UID_CAST_BASE(HTTPServer::Session)

HTTPServer::Session::Session()
{
}

HTTPServer::Session::~Session()
{
}

//
// HTTPServer::Redirecter
//

HTTPServer::Redirecter::Redirecter()
    : _port(-1)
{
}

bool HTTPServer::Redirecter::handleRequest(Request& request, Response& response)
{
    URLBuilder url(request.getURL());

    if (_port >= 0) {
        url.setPort(ToString(_port));
    }

    if (!_protocol.empty()) {
        url.setProtocol(_protocol);
    }

    response.setConnectionClose();
    response.redirect(url.toString().c_str());
    return true;
}

//
// HTTPServer::Router::Entry
//

int HTTPServer::Router::Entry::match(const URLPath& with, Value::Dictionary* arguments) const
{
    size_t withLength = with.getComponentCount();
    size_t ourLength = path.getComponentCount();

    size_t i;
    for (i = 0; i != ourLength; ++i) {
        StringView ourComponent = path.getComponent(i);
        StringView withComponent = i >= withLength ? StringView() : with.getComponent(i);

        if (!ourComponent.empty() && ourComponent[0] == '=') {
            if (ourComponent.size() >= 2 && ourComponent[1] == '=') {
                // Match the rest of the path. If we're being asked for the argument it means we're the winning
                // match, in which case give an accurate count of the path length. If not, exaggerate our match
                // length if we have additional path components we could consume.
                return Narrow<int>(arguments ? i : i + (i < withLength ? 1 : 0));
            }

            // Match a single path component.
            if (i >= withLength || withComponent.empty()) {
                return 0;
            }
            if (arguments) {
                // Most arguments are integers - try to store the argument as an int64, to save a memory allocation
                int64_t n;
                if (StringToInt(withComponent, n)) {
                    arguments->set(ourComponent.substr(1).to_string(), n);
                } else {
                    arguments->set(ourComponent.substr(1).to_string(), withComponent);
                }
            }

            continue;
        }

        if (i >= withLength || !StringsEqual(ourComponent, withComponent)) {
            return 0;
        }
    }

    return Narrow<int>(isRouter ? ourLength : (ourLength == withLength ? ourLength : 0));
}

//
// HTTPServer::CallbackHandler
//

bool HTTPServer::CallbackHandler::handleRequest(Request& request, Response& response)
{
    if (_callback) {
        return _callback(request, response);
    }

    return false;
}

//
// HTTPServer::Router
//

HTTPServer::Router::Router()
{
}

HTTPServer::Router::~Router()
{
}

void HTTPServer::Router::addFilter(const FilterCallback& filterCallback)
{
    _filters.push_back(filterCallback);
}

void HTTPServer::Router::route(URLPath path, HTTPMethod method, const HandlerCallback& handler)
{
    PRIME_ASSERT(handler);
    Entry entry;
    entry.path.swap(path);
    entry.method = method;
    entry.handlerCallback = handler;
    _entries.push_back(PRIME_MOVE(entry));
}

void HTTPServer::Router::route(URLPath path, const HandlerCallback& getHandler, const HandlerCallback& postHandler)
{
    if (getHandler) {
        route(path, HTTPMethodGet, getHandler);
    }
    if (postHandler) {
        route(path, HTTPMethodPost, postHandler);
    }
}

void HTTPServer::Router::route(URLPath path, HTTPMethod method, Handler* handler)
{
    Entry entry;
    entry.path.swap(path);
    entry.method = method;
    entry.handler = handler;
    entry.isRouter = false;
    _entries.push_back(PRIME_MOVE(entry));
}

void HTTPServer::Router::route(URLPath path, Router* router)
{
    Entry entry;
    entry.path.swap(path);
    entry.method = HTTPMethodUnknown;
    entry.handler = router;
    entry.isRouter = true;
    _entries.push_back(PRIME_MOVE(entry));
}

bool HTTPServer::Router::reroute(const URLPath& path, Request& request, Response& response)
{
    if (routeRequest(path, 0, request, response)) {
        return true;
    }

    URLPath fixed;

    // We can't convert a directory to a non-directory (and nginx doesn't either) or you end up with
    // infinite redirects.
    if (!path.isDirectory()) {
        fixed = path.toDirectory();
    }

    if (fixed.getComponentCount() > 0 && findEntry(fixed, request.getMethod())) {
        URL newURL(request.getURL());
        newURL.setPathComponents(fixed);
        response.redirect(newURL.toString().c_str());
        return true;
    }

    return false;
}

bool HTTPServer::Router::handleRequest(Request& request, Response& response)
{
    // TODO: is this allocating memory each time?
#ifdef PRIME_CXX11
    request.setRerouteCallback([this](const URLPath& path, Request& request, Response& response) -> bool {
        return this->reroute(path, request, response);
    });
#else
    request.setRerouteCallback(MethodCallback(this, &HTTPServer::Router::reroute));
#endif

    return reroute(request.getPath(), request, response);
}

bool HTTPServer::Router::routeRequest(const URLPath& path, int pathOffset, Request& request, Response& response)
{
    const URLPath* subPath;
    URLPath pathTail;
    if (pathOffset <= 0) {
        subPath = &path;
    } else {
        pathTail = path.getTail((unsigned int)pathOffset);
        subPath = &pathTail;
    }

    const Entry* best = findEntry(*subPath, request.getMethod());

    if (best) {
        Value::Dictionary arguments;
        // best->match returns a different value for == when it has an arguments argument
        int matchLength = best->match(*subPath, &arguments);
        request.setPathOffset(matchLength + pathOffset);
        if (request.isVerboseEnabled() && !arguments.empty() && !best->isRouter) {
            request.getLog()->trace(MakeString("Arguments: ", arguments));
        }
        request.mergeArguments(PRIME_MOVE(arguments));

        for (size_t i = 0; i != _filters.size(); ++i) {
            if (!_filters[i](request, response)) {
                return true;
            }
        }

        if (best->handlerCallback) {
            best->handlerCallback(request, response);
            return true;
        }

        if (best->handler) {
            if (best->isRouter) {
                return static_cast<Router*>(best->handler.get())->routeRequest(path, pathOffset + matchLength, request, response);
            } else {
                return best->handler->handleRequest(request, response);
            }
        }
    }

    return false;
}

const HTTPServer::Router::Entry* HTTPServer::Router::findEntry(const URLPath& path, HTTPMethod method) const
{
    const Entry* best = NULL;
    int bestLength = 0;

    for (size_t i = 0; i != _entries.size(); ++i) {
        const Entry& entry = _entries[i];

        if (entry.method == method || entry.method == HTTPMethodUnknown) {
            int length = entry.match(path, NULL);
            if (length > bestLength) {
                bestLength = length;
                best = &entry;
            }
        }
    }

    return best;
}

//
// HTTPServer::SessionManager
//

PRIME_DEFINE_UID_CAST_BASE(HTTPServer::SessionManager)

bool HTTPServer::SessionManager::generateSessionID(char* sid, size_t maxBytes, Log* log)
{
    static const char symbols[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    bool result = true;
#ifndef PRIME_NO_SECURERNG
    if (!SecureRNG().generateBytes(sid, maxBytes, log))
#endif
    {
        MersenneTwister mt;
        mt.seed(Clock::getLoopingMonotonicMilliseconds32() ^ 0x4c4d4c56);
        mt.generateBytes(sid, maxBytes, log);
        log->trace("Insecure session ID");
        result = false;
    }
    for (size_t i = 0; i != maxBytes; ++i) {
        uint8_t n = (uint8_t)sid[i];
        sid[i] = symbols[n % (sizeof(symbols) - 1)];
    }

    return result;
}

HTTPServer::SessionManager::SessionManager()
{
}

HTTPServer::SessionManager::~SessionManager()
{
}

HTTPServer::Router::FilterCallback HTTPServer::SessionManager::createFilter()
{
    return MethodCallback(this, &SessionManager::filter);
}

bool HTTPServer::SessionManager::filter(Request& request, Response& response)
{
    if (!request.getSession()) {
        request.setSession(getSession(request, &response, true));
    }

    return true;
}
}
