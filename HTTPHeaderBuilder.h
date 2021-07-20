// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_HTTPHEADERS_H
#define PRIME_HTTPHEADERS_H

#include "Config.h"
#include "Dictionary.h"
#include "HTTPParser.h"
#include "StreamBuffer.h"
#include "URL.h"

namespace Prime {

//
// HTTPHeaderBuilder
//

/// Send or receive HTTP request and response headers. It's almost always preferable to retrieve headers using an
/// HTTPParser, since it makes at most one copy of the headers (whereas HTTPHeaderBuilder makes a copy of every
/// header).
class PRIME_PUBLIC HTTPHeaderBuilder {
public:
    HTTPHeaderBuilder();

    ~HTTPHeaderBuilder();

    /// The size of the StreamBuffer's buffer determines the maximum size of the headers.
    /// protocol is only required if parsing requests and is used to correctly set up the URL.
    /// For new code, use an HTTPParser rather than using an HTTPHeaderBuilder if you only need to retrieve the
    /// headers.
    bool parse(HTTPParser::ParseMode mode, StreamBuffer* stream, Log* log, StringView protocol = "http");

    /// protocol is only required if parsing requests and is used to correctly set up the URL.
    /// For new code, use an HTTPParser rather than using an HTTPHeaderBuilder if you only need to retrieve the
    /// headers.
    bool parse(HTTPParser::ParseMode mode, StringView source, Log* log, StringView protocol = "http");

    /// Returns true if the connection was closed before any text was read.
    bool connectionWasClosed() const { return _closed; }

    /// Provides a Stream to read the body from a parsed request.
    RefPtr<Stream> readBody(StreamBuffer* networkStream, Log* log);

    /// Sends the request or response headers to the specified stream.
    bool send(Stream* stream, Log* log) const;

    /// Reset ready for a new request/response. parse() calls this for you, but you need to call it for a response.
    void reset();

    bool isKeepAlive() const;

    std::string getRawHeaders() const;

    /// Get a header by name.
    const std::string& get(StringView name) const { return _headers.get(name); }

    /// Get a header by name.
    const std::string& operator[](StringView name) const { return _headers.get(name); }

    /// Get all value for a header name.
    std::vector<std::string> getAll(StringView name) const { return _headers.getAll(name); }

    /// Find a header name.
    bool has(StringView name) const { return _headers.has(name); }

    /// Set a header by name.
    void set(StringView name, StringView value) { _headers.set(name, value); }

    /// Set a header to a UnixTime.
    void set(StringView name, const UnixTime& time);

    /// Add a value for a header name.
    void add(StringView name, StringView value) { _headers.add(name, value); }

    /// Remove a header by name.
    void remove(StringView name) { _headers.remove(name); }

    HTTPMethod getMethod() const { return _method; }
    void setMethod(HTTPMethod method) { _method = method; }

    bool isRequest() const { return _method != HTTPMethodUnknown; }
    bool isResponse() const { return _method == HTTPMethodUnknown; }

    const char* getMethodName() const { return GetHTTPMethodName(_method); }

    const std::string& getRequestURL() const { return _requestURL; }

    const URL& getURL() const { return _url; }
    void setURL(const URLBuilder& url) { _url = url; }
    void setURL(const URL& url) { _url = url; }
    void setURL(const URLView& view) { _url = view; }

    int getMajorVersion() const { return _majorVersion; }
    int getMinorVersion() const { return _minorVersion; }
    void setVersion(int major, int minor)
    {
        _majorVersion = major;
        _minorVersion = minor;
    }

    bool isVersionOrNewer(int major, int minor) const { return _majorVersion > major || (_majorVersion == major && _minorVersion >= minor); }

    int getResponseCode() const { return _responseCode; }
    void setResponseCode(int responseCode) { _responseCode = responseCode; }

    const std::string& getResponseCodeText() const { return _responseCodeText; }
    void setResponseCodeText(StringView responseCodeText) { StringCopy(_responseCodeText, responseCodeText); }

    void setResponse(int responseCode, StringView responseCodeText, bool keepalive = false);
    bool sendResponse(Stream* stream, Log* log, int responseCode, StringView responseCodeText,
        StringView data, bool keepalive = false);

private:
    /// Send only the headers, without the first line of the request or response.
    bool sendHeaders(Stream* stream, Log* log) const;

    bool loadFromParser(const HTTPParser& parser, Log* log, StringView protocol);

    URLDictionary _headers;
    HTTPMethod _method;
    URL _url;
    std::string _requestURL;
    int _majorVersion;
    int _minorVersion;
    int _responseCode;
    std::string _responseCodeText;
    bool _closed;
};
}

#endif
