// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_HTTP_H
#define PRIME_HTTP_H

#include "StringView.h"

namespace Prime {

//
// HTTP header parsing
//

PRIME_PUBLIC extern const char httpSeparators[];

/// token          = 1*<any CHAR except CTLs or separators>
/// separators     = "(" | ")" | "<" | ">" | "@"
///                | "," | ";" | ":" | "\" | <">
///                | "/" | "[" | "]" | "?" | "="
///                | "{" | "}" | SP | HT
PRIME_PUBLIC StringView HTTPParseToken(StringView text, StringView& remainingText);

/// quoted-string  = ( <"> *(qdtext | quoted-pair ) <"> )
/// qdtext         = <any TEXT except <">>
/// quoted-pair    = "\" CHAR
PRIME_PUBLIC std::string HTTPParseQuotedString(StringView text, StringView& remainingText);

PRIME_PUBLIC std::string HTTPParseTokenOrQuotedString(StringView text, StringView& remainingText);

PRIME_PUBLIC bool HTTPSkip(StringView text, StringView skip, StringView& remainingText);

//
// HTTP methods
//

enum HTTPMethod {
    HTTPMethodUnknown,
    HTTPMethodOptions,
    HTTPMethodGet,
    HTTPMethodHead,
    HTTPMethodPost,
    HTTPMethodPut,
    HTTPMethodDelete,
    HTTPMethodTrace,
    HTTPMethodConnect,
    HTTPMethodPatch
};

PRIME_PUBLIC const char* GetHTTPMethodName(HTTPMethod method);

PRIME_PUBLIC HTTPMethod GetHTTPMethodFromName(StringView method);

inline bool IsHTTP2xx(int code)
{
    return code >= 200 && code <= 299;
}

inline bool IsHTTPError(int code)
{
    return (code >= 400 && code <= 599) || code < 0;
}

//
// HTTPQValueParser
//

/// Parse the HTTP "q-value" headers (e.g., Accept and Accept-Encoding)
class PRIME_PUBLIC HTTPQValueParser {
public:
    static double getQValue(StringView headerValue, StringView name);

    HTTPQValueParser(StringView string)
        : _ptr(string.begin())
        , _end(string.end())
    {
    }

    struct QValue {
        StringView name;
        double q;

        QValue()
            : q(1.0)
        {
        }
    };

    /// Yields one QValue at a time and returns false when there is no more to parse.
    bool read(QValue& value);

private:
    const char* _ptr;
    const char* _end;
};

//
// HTTPCookieParser
//

/// Parse cookie name/value pairs from an HTTP Cookie header.
class PRIME_PUBLIC HTTPCookieParser {
public:
    HTTPCookieParser(StringView string)
        : _ptr(string.begin())
        , _end(string.end())
    {
    }

    struct Cookie {
        StringView name;
        StringView value;
    };

    bool read(Cookie& cookie);

private:
    const char* _ptr;
    const char* _end;
};
}

#endif
