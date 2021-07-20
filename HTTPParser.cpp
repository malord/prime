// Copyright 2000-2021 Mark H. P. Lord

#include "HTTPParser.h"
#include "StringUtils.h"
#include "TextEncoding.h"

namespace Prime {

HTTPParser::HTTPParser()
{
    reset();
}

HTTPParser::~HTTPParser()
{
}

void HTTPParser::reset()
{
    _method = HTTPMethodUnknown;
    _majorVersion = _minorVersion = -1;
    _closed = false;
    _responseCode = -1;
    _responseCodeText = StringView();
    _requestURL = URLView();
    _headers.clear();
    _copy.clear();
}

bool HTTPParser::parse(ParseMode mode, StreamBuffer* stream, bool copy, Log* log, bool dontAdvanceReadPointer, size_t* headersSize)
{
    reset();

    // Scan for the first non-whitespace then for \r\n\r\n, then pass everything up to and including that to
    // parse(begin, end) to parse as text.
    size_t at = 0;
    bool foundBeginning = false;
    while (!stream->getErrorFlag()) {
        const char* ptr = stream->getReadPointer() + at;
        const char* top = stream->getTopPointer();
        for (; ptr != top; ++ptr) {
            if (*ptr == '\r' && foundBeginning) {
                at = (size_t)(ptr - stream->getReadPointer());
                if (stream->matchBytes(at, "\r\n\r\n", 4, log)) {
                    if (!parse(mode, StringView((const char*)stream->getReadPointer(), (const char*)ptr + 4), copy, log)) {
                        return false;
                    }

                    if (!dontAdvanceReadPointer) {
                        stream->setReadPointer(ptr + 4);
                    }
                    if (headersSize) {
                        *headersSize = static_cast<size_t>((ptr + 4) - reinterpret_cast<const char*>(stream->getReadPointer()));
                    }
                    return true;
                }
            } else if (!ASCIIIsSpaceOrTabOrNewline(*ptr)) {
                foundBeginning = true;
            }
        }

        at = (size_t)(top - stream->getReadPointer());

        ptrdiff_t got = stream->fetchMore(log);
        if (got < 0) {
            return false;
        }

        if (got == 0) {
            if (stream->isEmpty()) {
                // log->trace(PRIME_LOCALISE("Connection closed while reading HTTP header."));
                _closed = true;
            } else {
                log->error(PRIME_LOCALISE("Invalid or too large HTTP header."));
            }
            return false;
        }
    }

    return false;
}

bool HTTPParser::parse(ParseMode mode, StringView source, bool copy, Log* log)
{
    reset();

    if (source.size() < 4 || memcmp(source.end() - 4, "\r\n\r\n", 4) != 0) {
        log->error(PRIME_LOCALISE("Invalid HTTP headers (not terminated by two CRLFs)."));
        return false;
    }

    if (copy) {
        StringCopy(_copy, source);
        source = _copy;
    }

    // Allow whitespace before the first line.
    const char* ptr = ASCIISkipSpacesTabsAndNewlines(source.begin(), source.end());
    const char* end = source.end();

    // Followed by the method (GET, POST, etc.).
    const char* space = ASCIISkipUntilSpaceOrTab(ptr, end);
    if (space == end) {
        log->error(PRIME_LOCALISE("Invalid HTTP headers."));
        return false;
    }

    _method = HTTPMethodUnknown;

    if (mode == ParseModeResponse && std::find(ptr, space, '/') == ptr + 4 && ASCIICompareIgnoringCase(ptr, "HTTP", 4) == 0) {
        // This is a response. Read the HTTP version, response code and response code string.

        ptr += 5;

        int major;
        if (!ParseInt(StringView(ptr, end), ptr, major, 10) || *ptr != '.') {
            log->error(PRIME_LOCALISE("Invalid HTTP response (invalid HTTP major version)."));
            return false;
        }
        int minor;
        if (!ParseInt(StringView(ptr + 1, end), ptr, minor, 10) || !ASCIIIsSpaceOrTabOrNewline(*ptr)) {
            log->error(PRIME_LOCALISE("Invalid HTTP response (invalid HTTP minor version)."));
            return false;
        }

        setVersion(major, minor);

        ptr = ASCIISkipSpacesAndTabs(ptr);

        int code;
        if (!ParseInt(StringView(ptr, end), ptr, code, 10) || !ASCIIIsSpaceOrTabOrNewline(*ptr)) {
            log->error(PRIME_LOCALISE("Invalid HTTP response (invalid response code)."));
            return false;
        }

        _responseCode = code;

        const char* reasonBegin = ASCIISkipSpacesAndTabs(ptr);
        const char* reasonEnd = reasonBegin;

        while (reasonEnd != end && !ASCIIIsNewline(*reasonEnd)) {
            ++reasonEnd;
        }

        if (reasonEnd == end) {
            log->error(PRIME_LOCALISE("Invalid HTTP response (nothing after reason phrase)."));
            return false;
        }

        _responseCodeText = StringView(reasonBegin, reasonEnd);

        ptr = reasonEnd;

    } else if (mode == ParseModeRequest) {
        // This is a request - read the method (GET, POST, etc.)

        ptrdiff_t n = space - ptr;

        switch (n) {
        case 3:
            if (ASCIICompareIgnoringCase(ptr, "GET", 3) == 0) {
                _method = HTTPMethodGet;
            } else if (ASCIICompareIgnoringCase(ptr, "PUT", 3) == 0) {
                _method = HTTPMethodPut;
            }
            break;

        case 4:
            if (ASCIICompareIgnoringCase(ptr, "HEAD", 4) == 0) {
                _method = HTTPMethodHead;
            } else if (ASCIICompareIgnoringCase(ptr, "POST", 4) == 0) {
                _method = HTTPMethodPost;
            }
            break;

        case 5:
            if (ASCIICompareIgnoringCase(ptr, "TRACE", 5) == 0) {
                _method = HTTPMethodTrace;
            } else if (ASCIICompareIgnoringCase(ptr, "PATCH", 5) == 0) {
                _method = HTTPMethodPatch;
            }
            break;

        case 6:
            if (ASCIICompareIgnoringCase(ptr, "DELETE", 6) == 0) {
                _method = HTTPMethodDelete;
            }
            break;

        case 7:
            if (ASCIICompareIgnoringCase(ptr, "OPTIONS", 7) == 0) {
                _method = HTTPMethodOptions;
            } else if (ASCIICompareIgnoringCase(ptr, "CONNECT", 7) == 0) {
                _method = HTTPMethodConnect;
            }
            break;
        }

        if (_method == HTTPMethodUnknown) {
            char method[64];
            StringCopy(method, sizeof(method), ptr, (size_t)(space - ptr));
            log->error(PRIME_LOCALISE("Unknown HTTP method: %s"), method);
            return false;
        }

        ptr = space;

        // Followed by whitespace, followed by the URL up until a space/tab.
        const char* urlBegin = ASCIISkipSpacesAndTabs(ptr);
        const char* urlEnd = urlBegin;
        while (!ASCIIIsSpaceOrTab(*urlEnd) && *urlEnd != '\r') {
            ++urlEnd;
        }

        // Skip extra slashes since URL parsing thinks they're a host name.
        // while (urlBegin != urlEnd && *urlBegin == '/' && urlBegin[1] == '/')
        //  ++urlBegin;

        _requestURL.parse(StringView(urlBegin, urlEnd));

        // Followed by whitespace, followed by "HTTP/".
        ptr = ASCIISkipSpacesAndTabs(urlEnd);
        if (ASCIICompareIgnoringCase(ptr, "HTTP/", 5) != 0) {
            log->error(PRIME_LOCALISE("Invalid HTTP request (missing HTTP/)."));
            return false;
        }

        // Followed by the version number ("1.1").
        ptr += 5;

        int major;
        if (!ParseInt(StringView(ptr, end), ptr, major, 10) || *ptr != '.') {
            log->error(PRIME_LOCALISE("Invalid HTTP request (invalid HTTP major version)."));
            return false;
        }
        int minor;
        if (!ParseInt(StringView(ptr + 1, end), ptr, minor, 10) || !ASCIIIsSpaceOrTabOrNewline(*ptr)) {
            log->error(PRIME_LOCALISE("Invalid HTTP request (invalid HTTP minor version)."));
            return false;
        }

        setVersion(major, minor);
    }

    ptr = ASCIISkipSpacesAndTabs(ptr, end);
    if (mode != ParseModeHeadersOnly && (ptr == end || !ASCIIIsNewline(*ptr))) {
        log->error(PRIME_LOCALISE("Invalid HTTP headers (extra text on first line)."));
        return false;
    }

    // Skip past any whitespace before the first header
    ptr = ASCIISkipSpacesTabsAndNewlines(ptr, end);

    // Parse the headers
    for (;;) {
        // Any whitespace will have been skipped by now.

        if (ptr >= end - 2) {
            // If we're within the last two bytes, we know they're CRLF.
            // We could have skipped them though if there are no headers.
            PRIME_ASSERT(ptr <= end);
            break;
        }

        const char* nameStart = ptr;

        while (ptr != end && *ptr != ':') {
            ++ptr;
        }

        Header header;
        header.name = StringViewRightTrim(StringView(nameStart, ptr));

        if (ptr != end) {
            ++ptr;
        }

        // Field values have to convert linear whitespace a space, so I build them up in a string.
        const char* valueStart = ptr;
        const char* nextHeader = end;
        for (; ptr != end; ++ptr) {
            if (ASCIIIsNewline(*ptr)) {
                const char* ptr2 = ASCIISkipNextNewline(ptr, end);

                // Could be linear whitespace. The \r\n's will not make it in to the value.
                if (ptr2 == end || !ASCIIIsSpaceOrTab(*ptr2)) {
                    nextHeader = ptr2;
                    break;
                }
            }
        }

        header.value = StringViewTrim(StringView(valueStart, ptr));
        ptr = nextHeader;

        _headers.push_back(header);
    }

    return true;
}

URL HTTPParser::getAbsoluteURL(StringView defaultProtocol) const
{
    // If this is a request, compute the full URL from the Host header.
    if (!isRequest()) {
        return URL();
    }

    URLView host(get("Host"), URLView::ParseOptions().setHostOnly());
    host.setPath("");
    host.setQuery("");
    host.setFragment("");
    host.setUsername("");
    host.setPassword("");
    host.setParameter("");

    if (host.getProtocol().empty()) {
        host.setProtocol(defaultProtocol);
    }

    return URL::resolve(host, _requestURL);
}

StringView HTTPParser::get(StringView name) const
{
    for (size_t i = _headers.size(); i-- > 0;) {
        if (equalHeaders(_headers[i].name, name)) {
            return _headers[i].value;
        }
    }

    return StringView();
}

std::vector<StringView> HTTPParser::getAll(StringView name) const
{
    std::vector<StringView> headers;

    for (size_t i = _headers.size(); i-- > 0;) {
        if (equalHeaders(_headers[i].name, name)) {
            headers.push_back(_headers[i].value);
        }
    }

    return headers;
}

bool HTTPParser::isKeepAlive() const
{
    StringView header = get("Connection");

    if (isVersionOrNewer(1, 1)) {
        // As of HTTP/1.1, default Connection is keep-alive. It used to be close.
        if (header.empty()) {
            return true;
        }
    }

    return equalHeaders(header, "keep-alive");
}

StringView HTTPParser::getEncodedCookie(StringView name) const
{
    // There can be multiple Cookie headers
    for (size_t i = 0; i != _headers.size(); ++i) {
        if (equalHeaders(_headers[i].name, "Cookie")) {
            HTTPCookieParser cp(_headers[i].value);
            HTTPCookieParser::Cookie cookie;
            while (cp.read(cookie)) {
                if (ASCIIEqualIgnoringCase(cookie.name, name)) {
                    return cookie.value;
                }
            }
        }
    }

    return StringView();
}

std::string HTTPParser::getCookie(StringView name) const
{
    return URLDecode(getEncodedCookie(name), URLDecodeFlagPlusesAsSpaces);
}
}
