// Copyright 2000-2021 Mark H. P. Lord

#include "HTTP.h"
#include "NumberParsing.h"
#include "StringUtils.h"

// This needs to be 0 (lenient) in real world use
#define STRICT_COOKIES 0

namespace Prime {

//
// HTTP header parsing
//

const char httpSeparators[] = "()<>@,;:\\\"/[]?={} \t";

// token          = 1*<any CHAR except CTLs or separators>
// separators     = "(" | ")" | "<" | ">" | "@"
//                | "," | ";" | ":" | "\" | <">
//                | "/" | "[" | "]" | "?" | "="
//                | "{" | "}" | SP | HT
StringView HTTPParseToken(StringView text, StringView& remainingText)
{
    const char* end = text.end();
    const char* begin = ASCIISkipWhitespace(text.begin(), end);
    const char* ptr = begin;

    for (; ptr != end; ++ptr) {
        if (strchr(httpSeparators, *ptr) || static_cast<unsigned char>(*ptr) < 32u) {
            break;
        }
    }

    remainingText = StringView(ptr, end);
    return StringView(begin, ptr);
}

// quoted-string  = ( <"> *(qdtext | quoted-pair ) <"> )
// qdtext         = <any TEXT except <">>
// quoted-pair    = "\" CHAR
std::string HTTPParseQuotedString(StringView text, StringView& remainingText)
{
    const char* end = text.end();
    const char* begin = ASCIISkipWhitespace(text.begin(), end);

    if (begin != end && *begin == '"') {
        ++begin;
        const char* ptr = begin;

        std::string output;

        for (; ptr != end; ++ptr) {
            if (*ptr == '"') {
                remainingText = StringView(ptr + 1, end);
                return output;
            }

            if (*ptr == '\\') {
                ++ptr;
                if (ptr == end) {
                    break;
                }
            }

            output += *ptr;
        }
    }

    remainingText = StringView();
    return "";
}

std::string HTTPParseTokenOrQuotedString(StringView text, StringView& remainingText)
{
    text = StringViewLeftTrim(text);
    if (text.empty()) {
        remainingText = StringView();
        return "";
    }

    if (text[0] == '"') {
        return HTTPParseQuotedString(text, remainingText);
    }

    return HTTPParseToken(text, remainingText).to_string();
}

bool HTTPSkip(StringView text, StringView skip, StringView& remainingText)
{
    const char* end = text.end();
    const char* ptr = ASCIISkipWhitespace(text.begin(), end);

    if (StringStartsWith(StringView(ptr, end), skip)) {
        remainingText = text.substr(skip.size());
        return true;
    }

    remainingText = text;
    return false;
}

//
// HTTP methods
//

static const char* const methodNames[] = {
    "unknown HTTP method",
    "OPTIONS",
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "TRACE",
    "CONNECT",
    "PATCH"
};

const char* GetHTTPMethodName(HTTPMethod method)
{
    if ((size_t)method > PRIME_COUNTOF(methodNames)) {
        return methodNames[0];
    }

    return methodNames[(size_t)method];
}

HTTPMethod GetHTTPMethodFromName(StringView method)
{
    for (size_t i = 0; i != PRIME_COUNTOF(methodNames); ++i) {
        if (ASCIIEqualIgnoringCase(method, methodNames[i])) {
            return (HTTPMethod)i;
        }
    }

    return HTTPMethodUnknown;
}

//
// HTTPQValueParser
//

double HTTPQValueParser::getQValue(StringView headerValue, StringView name)
{
    HTTPQValueParser qvp(headerValue);
    QValue q;
    while (qvp.read(q)) {
        if (ASCIIEqualIgnoringCase(q.name, name)) {
            return q.q;
        }
    }

    return 0.0;
}

bool HTTPQValueParser::read(QValue& value)
{
    const char* begin = _ptr;
    while (_ptr != _end && (*_ptr != ';' && *_ptr != ',')) {
        ++_ptr;
    }

    if (_ptr == begin) {
        return false;
    }

    value.q = 1.0;
    value.name = StringViewTrim(StringView(begin, _ptr));

    if (_ptr == _end) {
        return true;
    }

    if (*_ptr == ',') {
        ++_ptr;
        return true;
    }

    for (;;) {
        PRIME_ASSERT(*_ptr == ';');
        ++_ptr;

        const char* nameBegin = _ptr;

        while (_ptr != _end && (*_ptr != '=' && *_ptr != ';' && *_ptr != ',')) {
            ++_ptr;
        }

        const char* nameEnd = _ptr;

        const char* valueBegin;
        const char* valueEnd;

        if (_ptr != _end && *_ptr == '=') {
            ++_ptr;
            valueBegin = _ptr;

            while (_ptr != _end && (*_ptr != ',' && *_ptr != ';')) {
                ++_ptr;
            }

            valueEnd = _ptr;
        } else {
            valueBegin = valueEnd = nameBegin;
        }

        if (nameBegin == nameEnd) {
            break;
        }

        StringView propertyName(nameBegin, nameEnd);
        StringView propertyValue(valueBegin, valueEnd);

        if (ASCIIEqualIgnoringCase(propertyName, "q")) {
            double n;
            if (StringToReal(propertyValue, n)) {
                value.q = n;
            } else {
                value.q = 0.0; // invalid q value
            }
        } else {
            // Currently ignore every other parameter, but they could be stored.
        }

        if (_ptr == _end) {
            break;
        }

        if (*_ptr == ',') {
            ++_ptr;
            break;
        }
    }

    return true;
}

//
// HTTPCookieParser
//

namespace {

#if STRICT_COOKIES

    // Everything except control characters (<32, >=127), spaces, tabs and separators ()<>@,;:\\\"/[]?={}
    uint8_t isTokenChar[256 / 8] = {
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(11111010),
        PRIME_BINARY(01101100),
        PRIME_BINARY(11111111),
        PRIME_BINARY(00000011),
        PRIME_BINARY(11111110),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11000111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(01010111),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
    };

    inline unsigned int IsTokenChar(unsigned char ch)
    {
        return isTokenChar[ch / 8] & (1u << (ch & 7));
    }

    // Everything except control characters (<32, >= 127), spaces, \\\",;
    uint8_t isCookieOctet[256 / 8] = {
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(11111010),
        PRIME_BINARY(11101111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11110111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11101111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(01111111),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
    };

    inline unsigned int IsCookieOctet(unsigned char ch)
    {
        return isCookieOctet[ch / 8] & (1u << (ch & 7));
    }

#endif
}

bool HTTPCookieParser::read(Cookie& cookie)
{
    for (;;) {
        _ptr = ASCIISkipSpacesAndTabs(_ptr, _end);

        if (_ptr == _end) {
            return false;
        }

        if (*_ptr == ';') {
            ++_ptr;
            continue;
        }

        const char* nameStart = _ptr;

#if STRICT_COOKIES
        while (_ptr != _end && IsTokenChar(*_ptr)) {
            ++_ptr;
        }

        if (_ptr == nameStart) {
            ++_ptr;
            continue;
        }
#else
        while (_ptr != _end && *_ptr != ';' && *_ptr != '=') {
            ++_ptr;
        }

        _ptr = ASCIIReverseSkipSpacesAndTabs(nameStart, _ptr);
#endif

        cookie.name = StringView(nameStart, _ptr);

        _ptr = ASCIISkipSpacesAndTabs(_ptr, _end);

        if (_ptr != _end && *_ptr == '=') {
            _ptr = ASCIISkipSpacesAndTabs(_ptr + 1, _end);

            const char* valueStart = _ptr;
            const char* valueEnd;

            if (_ptr != _end && *_ptr == '"') {
                ++valueStart;

                do {
                    ++_ptr;
                } while (_ptr != _end && *_ptr != '"');

                valueEnd = _ptr;
                if (_ptr != _end) {
                    ++_ptr;
                }
            } else {
#if STRICT_COOKIES
                while (_ptr != _end && IsCookieOctet(*_ptr)) {
                    ++_ptr;
                }
#else
                while (_ptr != _end && *_ptr != ';') {
                    ++_ptr;
                }

                _ptr = ASCIIReverseSkipSpacesAndTabs(valueStart, _ptr);
#endif

                valueEnd = _ptr;
            }

            cookie.value = StringView(valueStart, valueEnd);

            _ptr = ASCIISkipSpacesAndTabs(_ptr, _end);

            if (_ptr != _end && *_ptr == ';') {
                ++_ptr;
            }

        } else if (_ptr != _end) {
            ++_ptr; // We either just skipped a ';' or something completely invalid.
        }

        return true;
    }
}
}
