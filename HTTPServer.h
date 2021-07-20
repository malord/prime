// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_HTTPSERVER_H
#define PRIME_HTTPSERVER_H

#include "Config.h"
#include "HTTPHeaderBuilder.h"
#include "Optional.h"
#include "Settings.h"
#include "StreamBuffer.h"
#ifndef PRIME_NO_EXCEPTIONS
#include <stdexcept>
#endif
#include "Callback.h"
#include "UnownedPtr.h"

namespace Prime {

class PrefixLog;

/// Parses HTTP requests and routes them to a Handler (of which Router is a subclass) for processing. Knows
/// nothing about networks or sockets. HTTPServer does not create threads, but serve() can be called from
/// multiple threads (HTTPSocketServer does this).
class PRIME_PUBLIC HTTPServer : public RefCounted {
public:
    class Request;
    class Response;
    class Session;

    static const char htmlContentType[];
    static const char jsonContentType[];
    static const char plainTextContentType[];
    static const char formURLEncodedContentType[];
    static const char multipartFormDataContentType[];

    static const char htmlContentTypeCharsetUTF8[];
    static const char jsonContentTypeCharsetUTF8[];
    static const char plainTextContentTypeCharsetUTF8[];

    /// A thread-safe key/value store that persists across requests.
    class PRIME_PUBLIC Session : public RefCounted {
        PRIME_DECLARE_UID_CAST_BASE(0x72fe7278, 0xe57643ea, 0x866671b1, 0xfed9f9d9)

    public:
        Session();

        virtual ~Session();

        virtual const char* getID() const = 0;

        virtual Value get(const char* key) const = 0;

        virtual void set(const char* key, const Value& value) = 0;

        virtual void remove(const char* key) = 0;

        virtual Value getAndRemove(const char* key) = 0;

        virtual Value::Dictionary toDictionary() const = 0;
    };

    /// Encapsulates an entire web request received by the server, including the headers, cookies, URL and
    /// query string parameters.
    class PRIME_PUBLIC Request {
    public:
#ifdef PRIME_CXX11_STL
        typedef std::function<bool(const URLPath& path, Request&, Response&)> RerouteCallback;
#else
        typedef Callback3<const URLPath&, Request&, Response&> RerouteCallback;
#endif

        bool isVerboseEnabled() const
        {
            return _verboseLevel >= 1;
        }
        bool isVeryVerboseEnabled() const { return _verboseLevel >= 2; }

        HTTPMethod getMethod() const { return _headers.getMethod(); }
        bool isGet() const { return getMethod() == HTTPMethodGet; }
        bool isPost() const { return getMethod() == HTTPMethodPost; }
        bool isPut() const { return getMethod() == HTTPMethodPut; }
        bool isDelete() const { return getMethod() == HTTPMethodDelete; }

        StringView getHeader(StringView name) const { return _headers.get(name); }

        const UnixTime& getTime() const { return _time; }

        const URL& getURL() const { return _url; }

        const URLPath& getPath() const { return _path; }

        /// A Log that can be used by the handler. Anything written to the Log will go to the server's log and
        /// not to the page, unless the server redirects it for debugging purposes.
        Log* getLog() const { return _log; }

        //
        // Request information
        //

        /// Returns true if the request is an AJAX request (made using an XMLHTTPRequest or similar).
        /// Requires the X-Requested-With header to be set, which it is by default by jQuery (except for
        /// cross-domain requests).
        bool isAJAXRequest() const;

        /// Returns true if the content type is application/json.
        bool isJSON() const { return isContentType(jsonContentType); }

        /// Returns true if the content type matches. Deals with parameters in the Content-Type header.
        bool isContentType(StringView contentType) const;

        /// Checks for any proxy headers that identify the real IP address of the client. Returns an empty string
        /// if no headers are found.
        StringView getRealIP() const;

        //
        // Query string (e.g., ?a=1&b=2&c=3)
        //

        /// Returns a decoded query string parameter value, or an empty string. UsegetOptionalQueryString if you
        /// need to determine whether a query string exists.
        std::string getQueryString(StringView name) const;

        Optional<std::string> getOptionalQueryString(StringView name) const;

        bool getQueryBool(StringView name, bool defaultValue = false) const { return ToBool(getQueryString(name), defaultValue); }

        int getQueryInt(StringView name, int defaultValue = 0) const { return ToInt(getQueryString(name), defaultValue); }

        int64_t getQueryInt64(StringView name, int64_t defaultValue = 0) const { return ToInt64(getQueryString(name), defaultValue); }

        double getQueryDouble(StringView name, double defaultValue = 0) const { return ToDouble(getQueryString(name), defaultValue); }

        /// Returns all decoded queries for a given name.
        std::vector<std::string> getQueryStringVector(StringView name) const;

        /// Returns all query string values as a dictionary.
        Value::Dictionary getQueriesDictionary() const;

        //
        // Parameters (e.g., ;a=1&b=2&c=3)
        //

        /// Returns a decoded parameter value.
        std::string getParameterString(StringView name) const;

        //
        // Cookies
        //

        /// Returns a URL-decoded cookie value.
        std::string getCookie(StringView name) const;

        /// Returns a raw cookie value.
        StringView getEncodedCookie(StringView name) const;

        //
        // Forms/JSON POSTs
        //

        /// Can only be called if isJSON() returns true.
        const Value& getJSON() const
        {
            PRIME_ASSERT(!_json.isUndefined());
            return _json;
        }

        /// Returns undefined if the form variable does not exist.
        const Value& getForm(StringView name) const { return _json.getDictionary().get(name); }

        /// A form is parsed in to a Value Dictionary, meaning keys become case sensitive and duplicate values
        /// aren't supported.
        const Value::Dictionary& getForm() const { return _json.getDictionary(); }

        const std::string& getFormString(StringView name) const { return getForm(name).getString(); }

        bool getFormBool(StringView name, bool defaultValue = false) const { return getForm(name).toBool(defaultValue); }

        int getFormInt(StringView name, int defaultValue = 0) const { return getForm(name).toInt(defaultValue); }

        int64_t getFormInt64(StringView name, int64_t defaultValue = 0) const { return getForm(name).toInt64(defaultValue); }

        double getFormDouble(StringView name, double defaultValue = 0) const { return getForm(name).toDouble(defaultValue); }

        std::vector<std::string> getFormStringVector(StringView name) const;

        //
        // Non-form POSTs
        //

        /// Returns a Stream capable of reading the body of a POST.
        RefPtr<Stream> getStream(StreamBuffer* rawStream, Log* log);

        /// Returns the Content-Length of the POST, or null if not supplied.
        Optional<int64_t> getContentLength() const;

        //
        // Accept-Encoding
        //

        double getAcceptEncoding(StringView name) const;

        //
        // Accept
        //

        /// Return value equals preference, in range [0, 1]
        double getAccept(StringView name) const;

        /// Return value equals preference, in range [0, 1]
        double getAcceptJSON() const { return _acceptJSON; }

        /// Return value equals preference, in range [0, 1]
        double getAcceptHTML() const { return _acceptHTML; }

        bool wantsJSON() const { return getAcceptJSON() > getAcceptHTML(); }

        //
        // Arguments
        // Additional name/value pairs that come from the router.
        //

        const Value& getArgument(StringView key) const { return _arguments[key]; }

        const std::string& getArgumentString(StringView key) const { return _arguments[key].getString(); }

        bool getArgumentBool(StringView name, bool defaultValue = false) const { return _arguments[name].toBool(defaultValue); }

        int getArgumentInt(StringView name, int defaultValue = 0) const { return _arguments[name].toInt(defaultValue); }

        unsigned int getArgumentUInt(StringView name, unsigned int defaultValue = 0) const { return _arguments[name].toUInt(defaultValue); }

        int64_t getArgumentInt64(StringView name, int64_t defaultValue = 0) const { return _arguments[name].toInt64(defaultValue); }

        double getArgumentDouble(StringView name, double defaultValue = 0) const { return _arguments[name].toDouble(defaultValue); }

        template <typename Key>
        bool hasArgument(const Key& key) const { return _arguments.has(key); }

        void mergeArguments(Value::Dictionary arguments);

        /// How much of the path returned by getPath() was matched.
        void setPathOffset(size_t pathOffset) { _pathOffset = pathOffset; }

        size_t getPathOffset() const { return _pathOffset; }

        /// Returns the remainder of a path (i.e., the "==" in an argument capture in a router).
        URLPath getRemainingPath() const;

        /// Returns the remainder of a path (i.e., the "==" in an argument capture in a router).
        std::string getRemainingPathString() const;

        //
        // Sessions
        //

        /// Set the Session for this request.
        void setSession(Session* session) { _session = session; }

        Session* getSession() const { return _session; }

        //
        // Reroute
        //

        void setRerouteCallback(RerouteCallback callback);

        bool canReroute() const;

        bool reroute(const URLPath& path, Request& request, Response& response);

    private:
        friend class HTTPServer;

        class PRIME_PUBLIC Options {
        public:
            Options();

            bool load(Settings* settings);

        private:
            // size_t _maxFormSizeInBytes;
            int _verboseLevel;
            size_t _multipartFormStreamBufferSize;
            size_t _multipartMaxHeaderSizeInBytes;
            size_t _multipartMaxPartSizeInBytes;

            friend class Request;
        };

        void init(const Options& options, const UnixTime& time, Log* log)
        {
            _verboseLevel = options._verboseLevel;
            _options = options;
            _expect100 = false;
            _time = time;
            _log = log;
        }

        /// The size of the StreamBuffer's buffer determines the maximum size of the headers.
        bool parse(StreamBuffer* stream, StringView protocol);

        /// If parse() returns false, call this to check whether it was because the connection was closed.
        bool connectionWasClosed() const { return _headers.connectionWasClosed(); }

        bool parse(StringView string, StringView protocol);

        /// This is called automatically by parse(stream), but not by parse(StringView). Only "form-url-encoded"
        /// forms are parsed, not multi-part forms. Call getForm() or getForm(name) to retrieve form fields.
        bool parseForm(StreamBuffer* stream);

        /// This is called automatically by parse(stream), but not by parse(StringView). Only
        /// "multipart/form-encoded" forms are parsed, not "x-www-form-urlencoded". Call getForm() or
        /// getForm(name) to retrieve form fields.
        bool parseMultipartFormData(StreamBuffer* rawStream);

        /// This is called automatically by parse(stream), but not by parse(StringView). Call getJSON() to
        /// retrieve the JSON.
        bool parseJSON(StreamBuffer* stream);

        bool isKeepAlive() const { return _headers.isKeepAlive(); }

        bool isInitialised() const { return _log.get() != NULL; }

        bool initFromHeaders(StringView protocol);

        void needAcceptTypes() const;

        UnownedPtr<Log> _log;
        HTTPParser _headers;
        URL _url;
        URLPath _path;
        Value _json;
        Value::Dictionary _arguments;
        int _verboseLevel;
        std::string _source;
        double _acceptJSON;
        double _acceptHTML;
        RefPtr<Session> _session;
        bool _expect100;
        size_t _pathOffset;
        UnixTime _time;
        Options _options;
        RerouteCallback _rerouteCallback;
    };

    /// A Response to be returned by a Request handler.
    class PRIME_PUBLIC Response {
    public:
        class PRIME_PUBLIC Options {
        public:
            Options();

            bool load(Settings* settings);

        private:
            size_t _responseBufferSize;
            bool _useZeroCopy;
            int _gzipDynamicContentSizeInBytes;
            int _gzipStaticContentSizeInBytes;
            int _gzipCompressInMemorySizeInBytes;
            bool _gzipChunked;
            int _gzipCompressionLevel;
            int _verboseLevel;

            friend class Response;
        };

        Response();

        ~Response();

        /// After calling this, you must call setRequest unless an invalid request was parsed.
        void init(StreamBuffer* stream, const UnixTime& time, const Options& options);

        /// The request is required for redirects and acceptable encodings. This should be set for you by the time
        /// you process the request.
        void setRequest(Request& request);

        /// Frees as much as possible and resets the response. Also logs the time since the response began.
        void close();

        bool isHeaderOnly() const { return _headerOnly; }

        //
        // Connection
        //

        /// Sets either Connection: close or Connection: keep-alive.
        void setKeepAlive(bool value);

        void setConnectionClose();

        void setConnectionKeepAlive();

        /// Changed by calling setConnectionClose(), setConnectionKeepAlive() or setKeepAlive().
        bool getKeepAlive() const { return _keepAlive; }

        void redirect(StringView path);

        //
        // Response code
        //

        /// e.g., setResponseCode(404), which will set the response code text to a default value.
        void setResponseCode(int responseCode);

        /// e.g., setResponseCode(404, "Not Found")
        void setResponseCode(int responseCode, StringView responseCodeText);

        /// Sets the response code to an error code and sets the response body to a default message. If the
        /// request was for JSON, responds with a dictionary { "error": what }
        void error(Request& request, int responseCode, StringView what = StringView());

        /// Sets the response code to an error code and sets the response body to an HTML message, ignoring
        /// the requested content type. This is only intended to be used in cases where a Request isn't available.
        void errorHTML(int responseCode, StringView what = StringView());

#ifdef PRIME_CXX11_STL
        typedef std::function<std::string()> ErrorLogCallback;
#else
        typedef Callback0<std::string> ErrorLogCallback;
#endif

        /// You can assign a callback to get additional Log details to display for a 500 error.
        void setErrorLogCallback(const ErrorLogCallback& callback);

        int getResponseCode() const { return _headers.getResponseCode(); }

        //
        // Headers
        //

        void setHeader(StringView name, StringView value);

        void setHeader(StringView name, const UnixTime& unixTime);

        void addHeader(StringView name, StringView value);

        void removeHeader(StringView name);

        StringView getHeader(StringView name) const { return _headers.get(name); }

        bool hasHeader(StringView name) const { return _headers.has(name); }

        std::vector<std::string> getHeaderValues(StringView name) const { return _headers.getAll(name); }

        //
        // Cookies
        //

        /// e.g., setCookie("Name=Value; Path=/; HTTPOnly");
        void setCookie(StringView cookie);

        /// e.g., setCookie("Name", "Value", Clock::getCurrentTime() + UnixTime::createDays(365));
        void setCookie(StringView name, StringView value, const UnixTime& expire, StringView path = "/");

        /// A session cookie expires at the end of the browser session.
        /// e.g., setSessionCookie("Name", "Value");
        void setSessionCookie(StringView name, StringView value, StringView path = "/");

        /// Sets an empty value and immediate expiration.
        void deleteCookie(StringView name, StringView path = "/");

        //
        // Content
        //

        /// Sets the Content-Type, Content-Length and content.
        void setContent(std::string content, StringView contentType);

        void appendContent(StringView string);

        /// Sets the Content-Type, Content-Length and content.
        void setHTML(std::string html);

        /// Prepends a DOCTYPE and wraps the html in an <html> tag and a <body> tag.
        void setHTMLBody(std::string html);

        /// Sets the Content-Type, Content-Length and content.
        void setJSON(const Value::Dictionary& dictionary);

        /// Sets the Content-Type, Content-Length and content.
        void setPlainText(std::string text);

        void clearContent();

        void setContentType(StringView type);

        void setContentLength(uint64_t length);

        /// HTTPFileServer calls this for you. You can call it for dynamic content to ask the client to cache it
        /// for a small amount of time. If you don't call this, (and don't otherwise set Cache-Control or Expires
        /// headers), the correct headers will be sent to disable caching.
        void setExpirationSeconds(int seconds);

        //
        // Send the response
        //

        /// Does nothing if the response has already been sent.
        bool send(Log* log);

        //
        // Streaming
        //

        class PRIME_PUBLIC SendStreamOptions {
        public:
            SendStreamOptions()
                : _alreadyCompressed(false)
                , _isRawDeflated(false)
                , _crc32(0x0badf00d)
            {
            }

            SendStreamOptions& setAlreadyCompressed(bool value)
            {
                _alreadyCompressed = value;
                return *this;
            }

            bool isAlreadyCompressed() const { return _alreadyCompressed || _isRawDeflated; }

            SendStreamOptions& setDoNotCompress(bool value = true)
            {
                _alreadyCompressed = value;
                return *this;
            }

            SendStreamOptions& setDoNotBuffer(bool value = true)
            {
                _doNotBuffer = value;
                return *this;
            }

            bool getDoNotBuffer() const { return _doNotBuffer; }

            SendStreamOptions& setRawDeflated(bool value)
            {
                _isRawDeflated = value;
                return *this;
            }

            bool isRawDeflated() const { return _isRawDeflated; }

            SendStreamOptions& setCRC32(uint32_t value)
            {
                _crc32 = value;
                return *this;
            }

            uint32_t getCRC32() const { return _crc32; }

        private:
            bool _alreadyCompressed;
            bool _isRawDeflated;
            uint32_t _crc32;
            bool _doNotBuffer;
        };

        /// If stream->size() returns successfully, calls sendStream(). Otherwise, calls sendStreamChunked().
        bool sendStream(Stream* stream, Log* log, const SendStreamOptions& options = SendStreamOptions());

        /// Sets the Content-Length header then sends the headers followed by the content of the stream. If
        /// processing a HEAD request, the stream is not sent.
        bool sendStream(Stream* stream, Stream::Offset size, Log* log, const SendStreamOptions& options = SendStreamOptions());

        /// Sets the Transfer-Encoding header then sends the headers followed by the content of the stream. If
        /// processing a HEAD request, the stream is not sent.
        bool sendStreamChunked(Stream* stream, Log* log, const SendStreamOptions& options = SendStreamOptions());

#ifndef PRIME_NO_ZLIB

        /// Send the stream using gzip Content-Encoding. If small enough, the stream is compressed in to memory
        /// and sent from there, otherwise the compressed data is sent with chunked encoding. If processing a HEAD
        /// request, the stream is not sent.
        bool compressAndSendStream(Stream* stream, Stream::Offset size, Log* log, const SendStreamOptions& options = SendStreamOptions());

        /// Send the stream using chunked Transfer-Encoding and gzip Content-Encoding. If processing a HEAD request,
        /// the stream is not sent.
        bool compressAndSendStreamChunked(Stream* stream, Log* log, const SendStreamOptions& options = SendStreamOptions());

#endif

        bool shouldGZip() const
        {
            return _acceptGZip && isGZipEnabled();
        }

        /// Returns a Stream through which you can write raw data. Call close() on the returned Stream to confirm it's
        /// written correctly.
        RefPtr<Stream> beginStream(uint64_t contentLength, Log* log);

        /// Returns a Stream through which you can write the response. Uses chunked encoding, so the size of the
        /// response does not need to be specified in advance. Call close() on the returned Stream to confirm it's
        /// written correctly.
        RefPtr<Stream> beginChunked(Log* log, const SendStreamOptions& options = SendStreamOptions());

    private:
        friend class HTTPServer;

        static const char* descriptionForHTTPResponseCode(int responseCode);

        void construct();

        bool sendGZipHeader(Stream* stream, Log* log);

        bool sendGZipFooter(Stream* stream, uint32_t originalSize, uint32_t crc32, Log* log);

#ifndef PRIME_NO_ZLIB
        bool gzip(Stream* out, Stream* in, Log* log, Stream::Offset* originalSize);
#endif

        bool isGZipEnabled() const
        {
            return _options._gzipCompressionLevel > 0;
        }

        Options _options;

        RefPtr<StreamBuffer> _stream;

        mutable UnixTime _time;

        bool _headerOnly;

        HTTPHeaderBuilder _headers;
        std::string _content;

        HTTPMethod _method;
        URL _url;
        bool _acceptGZip;

        bool _sent;
        bool _keepAlive;

        ErrorLogCallback _errorLogCallback;
    };

#ifndef PRIME_NO_EXCEPTIONS

    /// An exception which can be thrown by a request handler to send an HTTP error.
    class Error : public std::runtime_error {
    public:
        Error(int statusCode, const std::string& what = "")
            : std::runtime_error(what)
            , _statusCode(statusCode)
        {
        }

        int getStatusCode() const { return _statusCode; }

    private:
        int _statusCode;
    };

#endif

    /// An object which can be added to a Router.
    class PRIME_PUBLIC Handler : public RefCounted {
    public:
        virtual bool handleRequest(Request& request, Response& response) = 0;
    };

    /// A Handler which invokes a callback.
    class PRIME_PUBLIC CallbackHandler : public Handler {
    public:
#ifdef PRIME_CXX11_STL
        typedef std::function<bool(Request&, Response&)> Callback;
#else
        typedef Callback2<bool, Request&, Response&> Callback;
#endif

        CallbackHandler()
        {
        }

        explicit CallbackHandler(Callback callback)
            : _callback(callback)
        {
        }

        void setCallback(Callback callback)
        {
            _callback = callback;
        }

        virtual bool handleRequest(Request& request, Response& response) PRIME_OVERRIDE;

    private:
        Callback _callback;
    };

    /// Route requests by matching URL paths.
    class PRIME_PUBLIC Router : public Handler {
    public:
        Router();

        ~Router();

#ifdef PRIME_CXX11_STL
        typedef std::function<void(Request&, Response&)> HandlerCallback;
        typedef std::function<bool(Request&, Response&)> FilterCallback;
#else
        typedef Callback2<void, Request&, Response&> HandlerCallback;
        typedef Callback2<bool, Request&, Response&> FilterCallback;
#endif

        void addFilter(const FilterCallback& filterCallback);

        void route(URLPath path, HTTPMethod method, const HandlerCallback& handler);

        void route(URLPath path, const HandlerCallback& getHandler, const HandlerCallback& postHandler = HandlerCallback());

        void route(URLPath path, HTTPMethod method, Handler* handler);

        void route(URLPath path, Router* router);

        virtual bool handleRequest(Request& request, Response& response) PRIME_OVERRIDE;

        bool reroute(const URLPath& path, Request& request, Response& response);

    private:
        bool routeRequest(const URLPath& path, int pathOffset, Request& request, Response& response);

#if PRIME_MSC_AND_OLDER(1300)
    public:
#endif
        struct Entry {
            URLPath path;
            HTTPMethod method;

            RefPtr<Handler> handler;
            bool isRouter;
            HandlerCallback handlerCallback;

            int match(const URLPath& with, Value::Dictionary* arguments = NULL) const;
        };
#if PRIME_MSC_AND_OLDER(1300)
    private:
#endif

        const Entry* findEntry(const URLPath& path, HTTPMethod method) const;

        std::vector<Entry> _entries;
        std::vector<FilterCallback> _filters;
    };

    class PRIME_PUBLIC Redirecter : public Handler {
    public:
        Redirecter();

        /// Set the port to redirect to, or -1 to leave the port intact.
        void setPort(int value) { _port = value; }
        int getPort() const { return _port; }

        /// Set the protocol (e.g., "https") to redirect to, or blank to leave the protocol intact.
        void setProtocol(StringView value) { StringCopy(_protocol, value); }
        const std::string& getProtocol() const { return _protocol; }

        // Handler implementation
        virtual bool handleRequest(Request& request, Response& response) PRIME_OVERRIDE;

    private:
        int _port;
        std::string _protocol;

        PRIME_UNCOPYABLE(Redirecter);
    };

    class PRIME_PUBLIC SessionManager : public RefCounted {
        PRIME_DECLARE_UID_CAST_BASE(0x3b286725, 0xdc3942e1, 0xadf9929b, 0x7f316107)

    public:
        /// Always succeeds, but returns false if the session ID is not sufficiently random.
        static bool generateSessionID(char* sid, size_t maxBytes, Log* log);

        SessionManager();

        virtual ~SessionManager();

        virtual RefPtr<Session> getSession(Request& request, Response* response, bool create) = 0;

        virtual RefPtr<Session> getSessionByID(StringView sessionID) = 0;

        virtual RefPtr<Session> createTemporarySession(Log* log) = 0;

        virtual void deleteSession(Request& request, Response& response) = 0;

        virtual void flush(Log* log) = 0;

        /// Returns a filter that can be passed to a router to imbue Requests with Sessions.
        HTTPServer::Router::FilterCallback createFilter();

    protected:
        virtual bool filter(Request& request, Response& response);
    };

    HTTPServer();

    ~HTTPServer();

    bool init(Handler* handler, Log* log, Settings* settings);

    /// Returns true if the connection should process another request, otherwise returns false.
    bool serve(StreamBuffer* readBuffer, StreamBuffer* writeBuffer, const char* protocol, Log* log,
        PrefixLog* prefixLog, bool canKeepAlive, const Value::Dictionary* requestArguments = NULL);

    int getVerboseLevel() const { return _verboseLevel; }

private:
    void updateSettings(Settings* settings);

    RefPtr<Handler> _handler;
    RefPtr<Log> _log;
    RefPtr<Settings> _settings;

    Settings::Observer _settingsObserver;

    int _verboseLevel;

    Request::Options _requestOptions;
    Response::Options _responseOptions;

    PRIME_UNCOPYABLE(HTTPServer);
};
}

#endif
