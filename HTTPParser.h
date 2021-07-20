// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_HTTPPARSER_H
#define PRIME_HTTPPARSER_H

#include "HTTP.h"
#include "StreamBuffer.h"
#include "URL.h"

namespace Prime {

/// Parses HTTP headers from a request or response.
class PRIME_PUBLIC HTTPParser {
public:
    static bool equalHeaders(StringView a, StringView b) PRIME_NOEXCEPT
    {
        return ASCIIEqualIgnoringCase(a, b);
    }

    HTTPParser();

    ~HTTPParser();

    enum ParseMode {
        ParseModeRequest,
        ParseModeResponse,
        ParseModeHeadersOnly
    };

    /// The size of the StreamBuffer's buffer determines the maximum size of the headers.
    /// protocol is only required if parsing requests and is used to correctly set up the URL.
    /// If copy is true, the region containing the headers is copied to an internal buffer, which is necessary
    /// if the StreamBuffer will be read from again during the lifetime of this HTTPParser. The copy is a single
    /// memory allocation.
    bool parse(ParseMode mode, StreamBuffer* stream, bool copy, Log* log, bool dontAdvanceReadPointer = false, size_t* headersSize = NULL);

    /// protocol is only required if parsing requests and is used to correctly set up the URL.
    /// If copy is true, the region containing the headers is copied to an internal buffer, which is necessary
    /// if the source string won't be available for the full lifetime of this HTTPParser. The copy is a single
    /// memory allocation.
    bool parse(ParseMode mode, StringView source, bool copy, Log* log);

    /// Returns true if the connection was closed before any text was read.
    bool connectionWasClosed() const { return _closed; }

    HTTPMethod getMethod() const { return _method; }

    /// This will be a relative URL. You need to resolve this against the Host.
    const URLView& getRequestURL() const { return _requestURL; }

    URL getAbsoluteURL(StringView defaultProtocol = "http") const;

    int getResponseCode() const { return _responseCode; }

    StringView getResponseCodeText() const { return _responseCodeText; }

    bool isRequest() const { return _method != HTTPMethodUnknown; }
    bool isResponse() const { return _method == HTTPMethodUnknown; }

    int getMajorVersion() const { return _majorVersion; }
    int getMinorVersion() const { return _minorVersion; }

    bool isVersionOrNewer(int major, int minor) const { return _majorVersion > major || (_majorVersion == major && _minorVersion >= minor); }

    struct Header {
        StringView name;
        StringView value;
    };

    StringView get(StringView name) const;

    StringView operator[](StringView name) const { return get(name); }

    std::vector<StringView> getAll(StringView name) const;

    size_t getHeaderCount() { return _headers.size(); }

    template <typename Index>
    const Header& getHeader(Index index) const { return _headers[index]; }

    ArrayView<const Header> getAllHeaders() const { return ArrayView<const Header>(_headers.data(), _headers.size()); }

    bool isKeepAlive() const;

    /// May return an empty string.
    StringView getRawHeaders() const { return _copy; }

    StringView getEncodedCookie(StringView name) const;

    std::string getCookie(StringView name) const;

private:
    /// Reset ready to re-use this object.
    void reset();

    void setVersion(int major, int minor)
    {
        _majorVersion = major;
        _minorVersion = minor;
    }

    HTTPMethod _method;
    int _majorVersion;
    int _minorVersion;
    bool _closed;
    int _responseCode;
    StringView _responseCodeText;
    URLView _requestURL;

    std::vector<Header> _headers;

    std::string _copy;

    PRIME_UNCOPYABLE(HTTPParser)
};
}

#endif
