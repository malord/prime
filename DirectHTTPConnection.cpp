// Copyright 2000-2021 Mark H. P. Lord

#include "DirectHTTPConnection.h"
#include "ChunkedReader.h"
#include "ChunkedWriter.h"
#include "DirectSocketConnector.h"
#include "HTTPHeaderBuilder.h"
#include "Substream.h"
#include "UnclosableStream.h"
#include "UnownedPtr.h"
// #include "Spew.h"
//#include "Spew2.h"

#if PRIME_DIRECTHTTPCONNECTION_ENABLE_TRANSCRIPTS
#include "MultiStream.h"
#endif

namespace Prime {

//
// Connection
//

class DirectHTTPConnectionFactory::Connection : public HTTPConnection {
public:
    ~Connection();

    // HTTPConnection implementation.
    virtual void setMethod(StringView method) PRIME_OVERRIDE;
    virtual void setRequestHeader(StringView key, StringView value) PRIME_OVERRIDE;
    virtual void setRequestBody(Stream* stream) PRIME_OVERRIDE;
    virtual int sendRequest(Log* log) PRIME_OVERRIDE;
    virtual int getResponseCode() const PRIME_OVERRIDE;
    virtual StringView getResponseCodeText() const PRIME_OVERRIDE;
    virtual const URL& getResponseURL() const PRIME_OVERRIDE;
    virtual RefPtr<Stream> getResponseContentStream() const PRIME_OVERRIDE;
    virtual int64_t getResponseContentLength() const PRIME_OVERRIDE;
    virtual StringView getResponseContentType() const PRIME_OVERRIDE;
    virtual void close() PRIME_OVERRIDE;
    virtual StringView getResponseHeader(StringView key) PRIME_OVERRIDE;
    virtual std::vector<StringView> getResponseHeaders(StringView key) PRIME_OVERRIDE;
    virtual std::vector<StringView> getResponseHeaderNames() PRIME_OVERRIDE;

protected:
    explicit Connection(DirectHTTPConnectionFactory* factory, const URLView& url);
    friend class DirectHTTPConnectionFactory;

private:
    /// Also responsible for opening the connection.
    bool beginRequest(Log* log);

    /// Fails if we can't resend.
    bool prepareToResend(Log* log);

    bool discardResponse();

    RefPtr<Stream> beginWrite(Log* log, int64_t contentLength = -1);

    enum SpecialLengths {
        SpecialLengthChunked = -1,
        SpecialLengthClose = -2,
        SpecialLengthInvalid = -3
    };

    RefPtr<DirectHTTPConnectionFactory> _factory;

    RefPtr<StreamBuffer> _buffer;

    // If true, this connection's socket must not be used.
    bool _disconnect;

    int64_t _requestLength;
    HTTPHeaderBuilder _request;
    bool _requestSent;
    RefPtr<Substream> _requestSubstream;
    RefPtr<Stream> _requestBody;

    bool _isKeepAlive;

    HTTPParser _response;
    UnownedPtr<const char> _failed;
    int64_t _responseLength;
    RefPtr<Stream> _responseStream;
};

//
// DirectHTTPConnectionFactory::Host
//

DirectHTTPConnectionFactory::Host::Host()
{
}

DirectHTTPConnectionFactory::Host::Host(const URLView& url)
    : host(url.getHost().begin(), url.getHost().end())
    , protocol(url.getProtocol().begin(), url.getProtocol().end())
    , port(ToInt(url.getPort(), -1))
{
    if (port == -1) {
        if (protocol == "http") {
            port = 80;
        } else if (protocol == "https") {
            port = 443;
        }
    }
}

DirectHTTPConnectionFactory::Host::Host(const Host& copy)
    : host(copy.host)
    , protocol(copy.protocol)
    , port(copy.port)
{
}

#ifdef PRIME_COMPILER_RVALUEREF
DirectHTTPConnectionFactory::Host::Host(Host&& move) PRIME_NOEXCEPT : host(std::move(move.host)),
                                                                      protocol(std::move(move.protocol)),
                                                                      port(move.port)
{
}
#endif

bool DirectHTTPConnectionFactory::Host::operator<(const Host& rhs) const
{
    if (host < rhs.host) {
        return true;
    }
    if (host > rhs.host) {
        return false;
    }

    if (protocol < rhs.protocol) {
        return true;
    }
    if (protocol > rhs.protocol) {
        return false;
    }

    return port < rhs.port;
}

bool DirectHTTPConnectionFactory::Host::operator==(const Host& rhs) const
{
    return host == rhs.host && protocol == rhs.protocol && port == rhs.port;
}

//
// DirectHTTPConnectionFactory
//

DirectHTTPConnectionFactory::DirectHTTPConnectionFactory(int readTimeoutMilliseconds, int writeTimeoutMilliseconds)
{
    _mutex.init(Log::getGlobal(), "DirectHTTPConnectionFactory mutex");

    _maxRedirects = 10;
    _maxRetries = 2;

    _connector = PassRef(new DirectSocketConnector(readTimeoutMilliseconds, writeTimeoutMilliseconds));
}

void DirectHTTPConnectionFactory::setSocketConnector(SocketConnector* connector)
{
    _connector = connector;
}

RefPtr<HTTPConnection> DirectHTTPConnectionFactory::createConnection(const URLView& url, Log*)
{
    return PassRef(new Connection(this, url));
}

RefPtr<StreamBuffer> DirectHTTPConnectionFactory::connect(const URLView& url, bool& isKeepAlive, Log* log)
{
    // Check the pool.
    if (_mutex.isInitialised()) {
        RecursiveMutex::ScopedLock lock(&_mutex);

        Host find(url);
        if (find.port != -1) {
            KeepAlivesMap::iterator iter = _keepAlives.find(find);

            if (iter != _keepAlives.end()) {
                RefPtr<StreamBuffer> stream = PRIME_MOVE(iter->second);

                PRIME_SPEW2("%s: reusing connection", iter->first.host.c_str());
                _keepAlives.erase(iter);

                isKeepAlive = true;
                return stream;
            }
        }
    }

    isKeepAlive = false;

    const bool isHTTPS = StringsEqual(url.getProtocol(), "https");
    const int defaultPort = isHTTPS ? 443 : 80;

    RefPtr<NetworkStream> networkStream = _connector->connect(url.getHostWithPort().c_str(), defaultPort, log);
    if (!networkStream) {
        return NULL;
    }

    RefPtr<Stream> streamToBuffer = networkStream.get();

    if (isHTTPS) {
        if (!_sslCallback) {
            log->error(PRIME_LOCALISE("HTTPS not available."));
            return NULL;
        }

        streamToBuffer = _sslCallback(networkStream, log);
        if (!streamToBuffer) {
            return NULL;
        }
    }

#ifdef PRIME_DIRECTHTTPCONNECTION_ENABLE_TRANSCRIPTS
    if (_transcriptStream) {
        RefPtr<MultiStream> multiStream = PassRef(new MultiStream);
        multiStream->setReadMode(MultiStream::ReadModeWrite);
        multiStream->addStream(streamToBuffer);
        multiStream->addStream(_transcriptStream);
        multiStream->setReadStream(streamToBuffer);
        streamToBuffer = multiStream;
    }
#endif

    RefPtr<StreamBuffer> buffer = PassRef(new StreamBuffer);
    if (!buffer->init(streamToBuffer, bufferSize)) {
        log->error(PRIME_LOCALISE("Couldn't allocate buffer."));
        return NULL;
    }

    return buffer;
}

void DirectHTTPConnectionFactory::returnToPool(const URLView& url, StreamBuffer* stream)
{
    if (!_mutex.isInitialised()) {
        return;
    }

    if (!PRIME_GUARD(stream->getReadPointer() == stream->getTopPointer())) {
        return;
    }

    if (!PRIME_GUARD(!stream->isDirty())) {
        return;
    }

    RecursiveMutex::ScopedLock lock(&_mutex);
    PRIME_SPEW2("%s: returning connection to pool", url.getHost().c_str());

    Host host(url);
    if (host.port >= 0) {
        _keepAlives.insert(KeepAlivesMap::value_type(host, stream));
    }
}

//
// DirectHTTPConnectionFactory::Connection
//

DirectHTTPConnectionFactory::Connection::Connection(DirectHTTPConnectionFactory* factory, const URLView& url)
{
    _factory = factory;
    _responseLength = SpecialLengthInvalid;
    _requestLength = SpecialLengthInvalid;
    _requestSent = false;
    _disconnect = false;
    _failed.reset(NULL);

    _request.setURL(url);
    _request.set("Connection", "keep-alive");
}

DirectHTTPConnectionFactory::Connection::~Connection()
{
    close();
    _factory.release();
}

void DirectHTTPConnectionFactory::Connection::close()
{
    if (!_buffer) {
        return;
    }

    if (!_disconnect && ASCIIEqualIgnoringCase(_response["Connection"], "close")) {
        _disconnect = true;
        PRIME_SPEW2("%s: connection closed (due to Connection: close)", _request.getURL().getHost().c_str());
    }

    if (!_disconnect && _factory) {
        if (!discardResponse()) {
            _disconnect = true;
            PRIME_SPEW2("%s: connection closed due to discarded response: %s", _request.getURL().getHost().c_str());
        }

        if (!_disconnect) {
            _factory->returnToPool(_request.getURL(), _buffer);
        }
    }

    _buffer.release();
}

bool DirectHTTPConnectionFactory::Connection::discardResponse()
{
    bool result = true;

    if (_responseStream) {
        for (;;) {
            char buffer[1024];
            ptrdiff_t nread = _responseStream->read(buffer, sizeof(buffer), Log::getNullLog());
            if (nread < 0) {
                result = false;
                break;
            }
            if (nread == 0) {
                break;
            }
        }

        _responseStream.release();
    }

    return result;
}

void DirectHTTPConnectionFactory::Connection::setMethod(StringView method)
{
    PRIME_ASSERT(!_requestSent);
    _request.setMethod(GetHTTPMethodFromName(method));
}

void DirectHTTPConnectionFactory::Connection::setRequestHeader(StringView key, StringView value)
{
    PRIME_ASSERT(!_requestSent);
    _request.set(key, value);
}

StringView DirectHTTPConnectionFactory::Connection::getResponseHeader(StringView key)
{
    return _response[key];
}

std::vector<StringView> DirectHTTPConnectionFactory::Connection::getResponseHeaderNames()
{
    std::vector<StringView> names;
    ArrayView<const HTTPParser::Header> headers = _response.getAllHeaders();
    names.reserve(headers.size());
    for (size_t i = 0; i != headers.size(); ++i) {
        names.push_back(headers[i].name);
    }
    return names;
}

std::vector<StringView> DirectHTTPConnectionFactory::Connection::getResponseHeaders(StringView key)
{
    return _response.getAll(key);
}

void DirectHTTPConnectionFactory::Connection::setRequestBody(Stream* stream)
{
    _requestBody = stream;
}

bool DirectHTTPConnectionFactory::Connection::beginRequest(Log* log)
{
    PRIME_ASSERT(!_requestSent);

    if (_requestBody) {
        // beginWrite() calls beginRequest() (this method), so steal the body so we don't infinitely recurse
        RefPtr<Stream> body = _requestBody;
        _requestBody.release();

        int64_t size = body->getSize(log);

        RefPtr<Stream> stream = beginWrite(log, size < 0 ? -1 : size);
        if (!stream || !stream->copyFrom(body, log, -1, log) || !stream->close(log)) {

            _disconnect = true;
            _request.setResponseCode(invalidHTTPResponseCode);
            return false;
        }

        _requestBody = body;
        PRIME_ASSERT(_requestSent);
        return true;
    }

    if (_requestLength == SpecialLengthInvalid) {
        _requestLength = 0;
        // _request.set("Content-Length", "0"); // This causes some servers to think we have a body
    }

    _isKeepAlive = false;

    for (;;) {
    try_another_connection:
        _buffer = _factory->connect(_request.getURL(), _isKeepAlive, log);
        if (!_buffer) {
            return false;
        }

        if (!_request.send(_buffer, log) || !_buffer->flushWrites(log)) {
            if (_isKeepAlive) {
                log->trace("Unable to send request on keep-alive connection, retrying with another connection...");
                goto try_another_connection;
            }

            _disconnect = true;
            return false;
        }

        break;
    }

    _requestSent = true;
    return true;
}

bool DirectHTTPConnectionFactory::Connection::prepareToResend(Log* log)
{
    PRIME_ASSERT(_requestSent);

    if (_requestBody) {
        if (!PRIME_GUARD(_requestBody->rewind(log))) {
            return false;
        }
    }

    _requestSent = false;

    return true;
}

RefPtr<Stream> DirectHTTPConnectionFactory::Connection::beginWrite(Log* log, int64_t contentLength)
{
    PRIME_ASSERT(!_requestSent);

    if (contentLength < 0) {
        _request.set("Transfer-Encoding", "chunked");
        _requestLength = SpecialLengthChunked;
        if (!beginRequest(log)) {
            return NULL;
        }

        return PassRef(new ChunkedWriter(PassRef(new UnclosableStream(_buffer))));
    }

    _request.set("Content-Length", ToString(contentLength));
    _requestLength = contentLength;
    if (!beginRequest(log)) {
        return NULL;
    }

    _requestSubstream = PassRef(new Substream(PassRef(new UnclosableStream(_buffer)), 0, false, contentLength, log, false));

    return _requestSubstream;
}

int DirectHTTPConnectionFactory::Connection::sendRequest(Log* log)
{
    if (_failed) {
        return invalidHTTPResponseCode;
    }

    if (_response.getResponseCode() >= 0) {
        // Already received the response headers.
        return _response.getResponseCode();
    }

    int redirectCount = 0;
    int retryCount = 0;

    for (;;) {
        if (!_requestSent) {
            if (!beginRequest(log)) {
                _failed.reset("Unable to send request headers");
                return invalidHTTPResponseCode;
            }
        }

        // Make sure we've finished writing the content.
        if (_requestLength > 0) {
            // If _requestLength < 0 then we wrote chunked, in which case releasing the ChunkedWriter
            // will have completed the send.
            // If requestLength > 0 then we need to make sure the right number of bytes were written
            // to the Substream.

            if (_requestSubstream->getOffset() != _requestLength) {
                _disconnect = true;
                _requestSubstream.release();
                _buffer->close(Log::getNullLog());
                _failed.reset("Incomplete HTTP request body");
                log->error("%s", _failed.get());
                return invalidHTTPResponseCode;
            }

            _requestSubstream.release();
        }

        // Do we really need to copy?
        if (!_response.parse(HTTPParser::ParseModeResponse, _buffer, true, log)) {
            if (_response.connectionWasClosed() && (_isKeepAlive || retryCount < _factory->getMaxRetries())) {
                if (!_isKeepAlive) {
                    ++retryCount;
                }

                _buffer.release();

                if (prepareToResend(log)) {
                    continue;
                }
            }

            _disconnect = true;
            _failed.reset("Invalid response");
            return invalidHTTPResponseCode;
        }

        _responseLength = SpecialLengthInvalid;

        StringView transferEncoding = _response["Transfer-Encoding"];
        if (ASCIIEqualIgnoringCase(transferEncoding, "chunked")) {
            _responseLength = SpecialLengthChunked;

        } else if (!transferEncoding.empty() && !ASCIIEqualIgnoringCase(transferEncoding, "direct")) {
            _disconnect = true;
            _failed.reset("Unsupported Transfer-Encoding");
            log->error("%s", _failed.get());
            return invalidHTTPResponseCode;

        } else {
            bool isClose = ASCIIEqualIgnoringCase(_response["Connection"], "close");

            int64_t length;
            StringView contentLength = _response["Content-Length"];
            if (contentLength.empty()) {
                if (isClose) {
                    _responseLength = SpecialLengthClose;
                } else {
                    _responseLength = 0;
                }

            } else if (StringToInt(contentLength, length)) {
                _responseLength = length;

            } else {
                _disconnect = true;
                _failed.reset("Invalid content length");
                log->error("%s", _failed.get());
                return invalidHTTPResponseCode;
            }
        }

        switch (_responseLength) {
        case SpecialLengthChunked: {
            _responseStream = PassRef(new ChunkedReader(_buffer));
            break;
        }

        case SpecialLengthClose: {
            _responseStream = PassRef(new UnclosableStream(_buffer));
            break;
        }

        case SpecialLengthInvalid:
            _responseStream = NULL;
            break;

        default: {
            PRIME_ASSERT(_responseLength >= 0);

            _responseStream = PassRef(new Substream(_buffer, 0, false, _responseLength, log, false));
            break;
        }
        }

        if (_response.getResponseCode() == 301 || _response.getResponseCode() == 302) {
            if (!prepareToResend(log)) {
                _disconnect = true;
                return _response.getResponseCode();
            }

            // Give the connection back to the pool
            close();

            if (redirectCount++ >= _factory->getMaxRedirects()) {
                return _response.getResponseCode();
            }

            // Redirect!
            StringView location = _response.get("location");
            URL url = URL::resolve(_request.getURL(), location);
            //log->trace("Redirect: %s => %s", _request.getURL().toString().c_str(), url.toString().c_str());
            _request.setURL(url);
            continue;
        }

        return _response.getResponseCode();
    }
}

int DirectHTTPConnectionFactory::Connection::getResponseCode() const
{
    return _failed ? invalidHTTPResponseCode : _response.getResponseCode();
}

StringView DirectHTTPConnectionFactory::Connection::getResponseCodeText() const
{
    return _failed ? StringView(_failed.get()) : StringView(_response.getResponseCodeText());
}

const URL& DirectHTTPConnectionFactory::Connection::getResponseURL() const
{
    return _request.getURL();
}

RefPtr<Stream> DirectHTTPConnectionFactory::Connection::getResponseContentStream() const
{
    return _responseStream;
}

int64_t DirectHTTPConnectionFactory::Connection::getResponseContentLength() const
{
    switch (_responseLength) {
    case SpecialLengthChunked:
        return -1;

    case SpecialLengthClose:
        return -1;

    default:
        if (_responseLength >= 0) {
            return _responseLength;
        }
    }

    // Not called sendRequest?
    return -2;
}

StringView DirectHTTPConnectionFactory::Connection::getResponseContentType() const
{
    return _response["content-type"];
}
}
