// Copyright 2000-2021 Mark H. P. Lord

#include "NumberParsing.h"
#include "NumberUtils.h"
#include "StringUtils.h"
#include <string.h>
#if defined(PRIME_STRTOMAX_USES_ERRNO) || defined(PRIME_STRTOFLOATMAX_USES_ERRNO)
#include <errno.h>
#endif
#include <ctype.h>
#ifdef PRIME_NO_STRTOIMAX
#include <stdio.h>
#endif

namespace Prime {

//
// String to integers
//

namespace Private {

#if !defined(PRIME_NO_STRTOIMAX)

    static inline int AutoBase(StringView string) PRIME_NOEXCEPT
    {
        const char* ptr = string.begin();
        const char* end = string.end();

        if (ptr != end && (*ptr == '-' || *ptr == '+')) {
            ++ptr;
        }

        if (end - ptr >= 2 && *ptr == '0') {
            if (ptr[1] == 'x' || ptr[1] == 'X') {
                return 0;
            }
        }

        return 10;
    }

#endif

    intmax_t ParseIntMax(MaybeNullTerminatedStringView string, char*& endPtr, int base) PRIME_NOEXCEPT
    {
        if (string.empty() || isspace(string[0])) {
            endPtr = (char*)string.begin();
            return 0;
        }

#if !defined(PRIME_NO_STRTOIMAX)

        if (!string.null_terminated()) {
            char buffer[128];

            size_t n = Min<size_t>(string.size(), sizeof(buffer) - 1);
            memcpy(buffer, string.begin(), n);
            buffer[n] = 0;

            intmax_t result = ParseIntMax(buffer, endPtr, base);
            endPtr = (char*)string.begin() + (endPtr - buffer);
            return result;
        }

        if (base < 0) {
            base = AutoBase(string.to_view());
        }

#if defined(PRIME_STRTOMAX_USES_ERRNO)

        errno = 0;
        intmax_t result = PRIME_STRTOIMAX(string.c_str(), &endPtr, base);
        if ((result == INTMAX_MIN || result == INTMAX_MAX) && errno != 0) {
            // Overflow is treated as an invalid number.
            endPtr = (char*)string.begin();
        }

        return result;

#else

        return PRIME_STRTOIMAX(string.c_str(), &endPtr, base);

#endif

#else

        const char* ptr = string.begin();
        const char* end = string.end();

        bool negative;
        if (ptr != end) {
            if (*ptr == '-') {
                ++ptr;
                negative = true;
            } else {
                if (*ptr == '+') {
                    ++ptr;
                }
                negative = false;
            }
        }

        uintmax_t u = ParseUIntMax(ptr, endPtr, base);
        if (endPtr == ptr) {
            endPtr = (char*)string.begin();
            return 0;
        }

        if (negative) {
            if (u > (uintmax_t)INTMAX_MAX + 1u) {
                endPtr = (char*)string.begin();
                return 0;
            }

            u = 0 - u;

        } else {
            if (u > (uintmax_t)INTMAX_MAX) {
                endPtr = (char*)string.begin();
                return 0;
            }
        }

        return (intmax_t)u;

#endif
    }

    uintmax_t ParseUIntMax(MaybeNullTerminatedStringView string, char*& endPtr, int base) PRIME_NOEXCEPT
    {
        if (string.empty() || isspace(string[0])) {
            endPtr = (char*)string.begin();
            return 0;
        }

#if !defined(PRIME_NO_STRTOIMAX)

        if (!string.null_terminated()) {
            char buffer[128];

            size_t n = Min<size_t>(string.size(), sizeof(buffer) - 1);
            memcpy(buffer, string.begin(), n);
            buffer[n] = 0;

            uintmax_t result = ParseUIntMax(buffer, endPtr, base);
            endPtr = (char*)string.begin() + (endPtr - buffer);
            return result;
        }

        if (base < 0) {
            base = AutoBase(string.to_view());
        }

#if defined(PRIME_STRTOMAX_USES_ERRNO)

        errno = 0;
        uintmax_t result = PRIME_STRTOUMAX(string.c_str(), &endPtr, base);
        if (result == UINTMAX_MAX && errno != 0) {
            // Overflow is treated as an invalid number.
            endPtr = (char*)string.begin();
        }

        return result;

#else

        return PRIME_STRTOUMAX(string, &endPtr, base);

#endif

#else

        uintmax_t u = 0;

        const char* ptr = string.begin();
        const char* end = string.end();

        if (base <= 0 && ptr != end) {
            if (*ptr == '0' && ptr + 1 != end) {
                if (ptr[1] == 'x' || ptr[1] == 'X') {
                    ptr += 2;
                    base = 16;

                } else if (base == 0) {
                    base = 8;

                } else {
                    base = 10;
                }

            } else {
                base = 10;
            }
        }

        uintmax_t ubase = (uintmax_t)base;
        const char* start2 = ptr;

        for (; ptr != end; ++ptr) {
            uintmax_t dig;

            if (*ptr >= '0' && *ptr <= '9') {
                dig = *ptr - '0';
            } else if (*ptr >= 'a' && *ptr <= 'z') {
                dig = *ptr - 'a' + 10;
            } else if (*ptr >= 'A' && *ptr <= 'Z') {
                dig = *ptr - 'A' + 10;
            } else {
                break;
            }

            if (dig >= base) {
                break;
            }

            u *= ubase;
            u += dig;
        }

        // 0x is stripped from the string but a leading zero signifying octal is not
        if (ptr == start2) {
            endPtr = (char*)string.begin();
            return 0;
        }

        endPtr = (char*)ptr;
        return u;

#endif
    }
}

//
// String to float
//

namespace Private {

    FloatMax ParseFloatMax(MaybeNullTerminatedStringView string, char*& endPtr) PRIME_NOEXCEPT
    {
        if (string.empty() || isspace(string[0])) {
            endPtr = (char*)string.begin();
            return 0;
        }

        if (!string.null_terminated()) {
            char buffer[128];

            size_t n = Min<size_t>(string.size(), sizeof(buffer) - 1);
            memcpy(buffer, string.begin(), n);
            buffer[n] = 0;

            FloatMax result = ParseFloatMax(buffer, endPtr);
            endPtr = (char*)string.begin() + (endPtr - buffer);
            return result;
        }

#ifdef PRIME_STRTOFLOATMAX_USES_ERRNO

        errno = 0;
        char* ptr;
        FloatMax result = PRIME_STRTOFLOATMAX(string.c_str(), &ptr);
        if (errno != 0) {
            endPtr = (char*)string.begin();
        } else {
            endPtr = ptr;
        }
        return result;

#else

        char* ptr;
        FloatMax result = PRIME_STRTOFLOATMAX(string.c_str(), &ptr);
        endPtr = ptr;
        return result;

#endif
    }

}
}
