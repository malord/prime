// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_NUMBERPARSING_H
#define PRIME_NUMBERPARSING_H

#include "StringUtils.h"

namespace Prime {

//
// Strings to integers
//

namespace Private {
    /// Converts a string to an intmax_t and set *endPtr to point to the first non-number character in the string.
    /// If base is zero, it is autocomputed as hex or octal based on the prefix (0x or 0, respectively).
    /// If base is negative, it is autocomputed as hex but not octal.
    /// Leading whitespace is an error.
    PRIME_PUBLIC intmax_t ParseIntMax(MaybeNullTerminatedStringView string, char*& endPtr, int base) PRIME_NOEXCEPT;

    /// Converts a string to a uintmax_t and set *endPtr to point to the first non-number character in the string.
    /// If base is zero, it is autocomputed as hex or octal based on the prefix (0x or 0, respectively).
    /// If base is negative, it is autocomputed as hex but not octal.
    /// Leading whitespace is an error.
    PRIME_PUBLIC uintmax_t ParseUIntMax(MaybeNullTerminatedStringView string, char*& endPtr, int base) PRIME_NOEXCEPT;
}

template <bool IsSigned>
struct ParseIntPolicy {
};

template <>
struct ParseIntPolicy<true> {

    typedef intmax_t Type;

    static Type parseInt(MaybeNullTerminatedStringView string, char*& endPtr, int base) PRIME_NOEXCEPT
    {
        return Private::ParseIntMax(string, endPtr, base);
    }
};

template <>
struct ParseIntPolicy<false> {

    typedef uintmax_t Type;

    static Type parseInt(MaybeNullTerminatedStringView string, char*& endPtr, int base) PRIME_NOEXCEPT
    {
        return Private::ParseUIntMax(string, endPtr, base);
    }
};

/// Convert a string to an integer. Returns true if a valid integer was parsed. Skips ASCII whitespace at the
/// beginning of the string  and sets endPtr to the first character after the integer. Supports both signed and
/// unsigned integer types.
template <typename Integer>
bool ParseInt(MaybeNullTerminatedStringView string, const char*& endPtr, Integer& integer, int base = -1) PRIME_NOEXCEPT
{
    string.remove_prefix((size_t)(ASCIISkipWhitespace(string.begin(), string.end()) - string.begin()));

    typedef ParseIntPolicy<Integer(-1) < Integer(0)> Policy;

    char* ptr;
    typename Policy::Type result = Policy::parseInt(string, ptr, base);

    if (ptr == string.begin()) {
        return false;
    }

    // Check for truncation.
    Integer cast = static_cast<Integer>(result);
    if (cast != result) {
        return false;
    }

    endPtr = ptr;
    integer = cast;
    return true;
}

/// Convert a string to an integer. Returns true if a valid integer was parsed. This variant of the function
/// requires that there be nothing other than whitespace after the last integer.
template <typename Integer>
bool StringToInt(MaybeNullTerminatedStringView string, Integer& integer, int base = -1) PRIME_NOEXCEPT
{
    const char* endPtr;
    if (!ParseInt(string, endPtr, integer, base)) {
        return false;
    }

    return ASCIISkipWhitespace(endPtr, string.end()) == string.end();
}

/// Decode a string that contains a series of integers. Skips whitespace before each integer and skips whitespace
/// and an optional comma after every number except the last. You can specify the minimum and maximum number of
/// floats to be parsed, and the number actually parsed is stored in count.
template <typename Integer>
bool ParseIntArray(MaybeNullTerminatedStringView string, const char*& endPtr, Integer* array,
    size_t minCount, size_t maxCount, size_t* countOut, int base = -1) PRIME_NOEXCEPT
{
    size_t countTemp;
    size_t& count = countOut ? *countOut : countTemp;
    count = 0;
    for (; maxCount--; ++array, ++count) {
        string.remove_prefix((size_t)(ASCIISkipWhitespaceForArray(string.begin(), string.end(), count == 0 ? 0 : ',') - string.begin()));

        if (string.empty()) {
            if (count >= minCount) {
                break;
            }
        }

        const char* ptr;
        if (!ParseInt<Integer>(string, ptr, *array, base)) {
            return false;
        }

        string.remove_prefix((size_t)(ptr - string.begin()));
    }

    endPtr = string.begin();
    return true;
}

/// Decode a string that contains a series of integers. Skips whitespace before each integer and skips whitespace
/// and an optional comma after every integer except the last.
template <typename Integer>
bool ParseIntArray(MaybeNullTerminatedStringView string, const char*& endPtr, Integer* array, size_t count,
    int base = -1) PRIME_NOEXCEPT
{
    size_t got;
    return ParseIntArray(string, endPtr, array, count, count, &got, base) && got == count;
}

/// Decode a string that contains a series of integers. Skips whitespace before each integer and skips whitespace
/// and an optional comma after every integer except the last. This variant of the function requires that there be
/// nothing other than whitespace after the last integer.
template <typename Integer>
bool StringToIntArray(MaybeNullTerminatedStringView string, Integer* array, size_t minCount, size_t maxCount,
    size_t* count, int base = -1) PRIME_NOEXCEPT
{
    const char* endPtr;
    if (!ParseIntArray(string, endPtr, array, minCount, maxCount, count, base)) {
        return false;
    }

    return ASCIISkipWhitespace(endPtr, string.end()) == string.end();
}

/// Decode a string that contains a series of integers. Skips whitespace before each integer and skips whitespace
/// and an optional comma after every integer except the last. This variant of the function requires that there be
/// nothing other than whitespace after the last integer.
template <typename Integer>
bool StringToIntArray(MaybeNullTerminatedStringView string, Integer* array, size_t count, int base = -1) PRIME_NOEXCEPT
{
    const char* endPtr;
    if (!ParseIntArray(string, endPtr, array, count, base)) {
        return false;
    }

    return ASCIISkipWhitespace(endPtr, string.end()) == string.end();
}

/// Decode a number from an ASCII encoded string where the number is in octal. On error (not an octal number),
/// returns false.
template <typename Integer>
bool ParseOctInt(StringView string, const char*& endPtr, Integer& value) PRIME_NOEXCEPT
{
    const char* ptr = string.begin();
    const char* end = string.end();

    Integer n = 0;

    for (; ptr != end; ++ptr) {
        if (*ptr >= '0' && *ptr <= '7') {
            n = (n << 3) | Integer(*ptr - '0');
        } else {
            break;
        }
    }

    if (ptr == string.begin()) {
        return false;
    }

    endPtr = ptr;
    value = n;
    return true;
}

/// Decode a number from an ASCII encoded string where the number is in hexadecimal. On error (not a hexadecimal
/// number), returns false.
template <typename Integer>
bool ParseHexInt(StringView string, const char*& endPtr, Integer& value) PRIME_NOEXCEPT
{
    const char* ptr = string.begin();
    const char* end = string.end();

    Integer n = 0;

    for (; ptr != end; ++ptr) {
        if (*ptr >= '0' && *ptr <= '9') {
            n = (n << 4) | Integer(*ptr - '0');
        } else if (*ptr >= 'a' && *ptr <= 'f') {
            n = (n << 4) | Integer(*ptr - 'a' + 10);
        } else if (*ptr >= 'A' && *ptr <= 'F') {
            n = (n << 4) | Integer(*ptr - 'A' + 10);
        } else {
            break;
        }
    }

    if (ptr == string.begin()) {
        return false;
    }

    endPtr = ptr;
    value = n;
    return true;
}

//
// Integers to strings
//

template <typename Integer>
char* UnsignedIntToString(char* dest, char* destEnd, Integer value, int digitGroup = 0, char groupSeparator = ',') PRIME_NOEXCEPT
{
    char digits[50];
    int ndigits = 0;
    while (value != 0 && ndigits != PRIME_COUNTOF(digits)) {
        digits[ndigits++] = (char)((value % 10) + '0');
        value /= 10;
    }

    if (!ndigits) {
        digits[ndigits++] = '0';
    }

    if (!digitGroup) {
        while (ndigits--) {
            if (dest < destEnd) {
                *dest = digits[ndigits];
            }
            ++dest;
        }
    } else {
        int nextSeparator = ndigits % digitGroup;
        if (!nextSeparator) {
            nextSeparator = digitGroup;
        }

        while (ndigits--) {
            if (nextSeparator == 0) {
                nextSeparator = digitGroup - 1;
                if (dest < destEnd) {
                    *dest = groupSeparator;
                }
                ++dest;
            } else {
                --nextSeparator;
            }

            if (dest < destEnd) {
                *dest = digits[ndigits];
            }
            ++dest;
        }
    }

    return dest;
}

template <typename Integer>
inline size_t UnsignedIntToString(char* buffer, size_t bufferSize, Integer value, int digitGroup = 0, char groupSeparator = ',') PRIME_NOEXCEPT
{
    if (buffer && !PRIME_GUARD(bufferSize)) {
        buffer = NULL;
    }

    char* dest = buffer;
    char* destEnd = buffer ? buffer + bufferSize - 1 : NULL;

    dest = UnsignedIntToString(dest, destEnd, value, digitGroup, groupSeparator);

    if (buffer) {
        *dest = '\0';
    }

    return (size_t)(dest - buffer);
}

#ifdef PRIME_CXX11_STL

template <typename Integer>
inline char* IntToString(char* dest, char* destEnd, Integer value, int digitGroup = 0, char groupSeparator = ',') PRIME_NOEXCEPT
{
    typedef typename std::make_unsigned<Integer>::type UnsignedInteger;
    UnsignedInteger uvalue;

    if (value < 0) {
        if (dest < destEnd) {
            *dest = '-';
        }
        ++dest;

        uvalue = (UnsignedInteger)0 - (UnsignedInteger)value; // Cast will sign-extend.
    } else {
        uvalue = value;
    }

    return UnsignedIntToString(dest, destEnd, uvalue, digitGroup, groupSeparator);
}

#else

template <typename Integer>
inline char* IntToString(char* dest, char* destEnd, Integer value, int digitGroup = 0, char groupSeparator = ',') PRIME_NOEXCEPT
{
    // If performance is a problem you'll have to simulate std::make_unsigned. Specialising for char, short
    // and int should be enough leaving the default as uintmax_t.
    uintmax_t uvalue;

    if (value < 0) {
        if (dest < destEnd) {
            *dest = '-';
        }
        ++dest;

        uvalue = (uintmax_t)0 - (uintmax_t)value; // Cast will sign-extend.
    } else {
        uvalue = value;
    }

    return UnsignedIntToString(dest, destEnd, uvalue, digitGroup, groupSeparator);
}

#endif

template <typename Integer>
inline size_t IntToString(char* buffer, size_t bufferSize, Integer value, int digitGroup = 0, char groupSeparator = ',') PRIME_NOEXCEPT
{
    if (buffer && !PRIME_GUARD(bufferSize)) {
        buffer = NULL;
    }

    char* dest = buffer;
    char* destEnd = buffer ? buffer + bufferSize - 1 : NULL;

    dest = IntToString(dest, destEnd, value, digitGroup, groupSeparator);

    if (buffer) {
        *dest = '\0';
    }

    return (size_t)(dest - buffer);
}

template <typename Integer>
inline bool UnsignedIntToStringWithThousandsSeparator(char* buffer, size_t bufferSize, Integer number, char separator) PRIME_NOEXCEPT
{
    return UnsignedIntToString(buffer, bufferSize, number, 3, separator) <= bufferSize;
}

template <typename Integer>
inline bool IntToStringWithThousandsSeparator(char* buffer, size_t bufferSize, Integer number, char separator) PRIME_NOEXCEPT
{
    return IntToString(buffer, bufferSize, number, 3, separator) <= bufferSize;
}

//
// Strings to floating point
//

namespace Private {
    /// Leading whitespace is an error.
    PRIME_PUBLIC FloatMax ParseFloatMax(MaybeNullTerminatedStringView string, char*& endPtr) PRIME_NOEXCEPT;
}

/// Convert a string to a floating point number. Returns true if a valid number was parsed. Skips whitespace at
/// the beginning of the string and sets *endPtr to the first character after the number.
template <typename Real>
bool ParseReal(MaybeNullTerminatedStringView string, const char*& endPtr, Real& number) PRIME_NOEXCEPT
{
    string.remove_prefix((size_t)(ASCIISkipWhitespace(string.begin(), string.end()) - string.begin()));

    char* ptr;
    FloatMax result = Private::ParseFloatMax(string, ptr);
    if (ptr == string.begin()) {
        return false;
    }

    endPtr = ptr;
    number = static_cast<Real>(result);
    return true;
}

/// Convert a string to a floating point number. Returns true if a valid number was parsed. This variant of the
/// function requires that there be nothing other than whitespace after the last number.
template <typename Real>
bool StringToReal(MaybeNullTerminatedStringView string, Real& number) PRIME_NOEXCEPT
{
    const char* endPtr;
    if (!ParseReal(string, endPtr, number)) {
        return false;
    }

    return ASCIISkipWhitespace(endPtr, string.end()) == string.end();
}

/// Decode a string that contains a series of floats. Skips whitespace before each number and skips whitespace and
/// an optional comma after every number except the last. You can specify the minimum and maximum number of floats
/// to be parsed, and the number actually parsed is stored in *count.
template <typename Real>
bool ParseRealArray(MaybeNullTerminatedStringView string, const char*& endPtr, Real* array, size_t minCount,
    size_t maxCount, size_t* countOut) PRIME_NOEXCEPT
{
    size_t countTemp;
    size_t& count = countOut ? *countOut : countTemp;
    count = 0;
    for (; maxCount--; ++array, ++count) {
        string.remove_prefix((size_t)(ASCIISkipWhitespaceForArray(string.begin(), string.end(), count == 0 ? 0 : ',') - string.begin()));

        if (string.empty()) {
            if (count >= minCount) {
                break;
            }
        }

        const char* ptr;
        if (!ParseReal<Real>(string, ptr, *array)) {
            return false;
        }

        string.remove_prefix((size_t)(ptr - string.begin()));
    }

    endPtr = string.begin();
    return true;
}

/// Decode a string that contains a series of floats. Skips whitespace before each number and skips whitespace and
/// an optional comma after every number except the last.
template <typename Real>
bool ParseRealArray(MaybeNullTerminatedStringView string, const char*& endPtr, Real* array, size_t count) PRIME_NOEXCEPT
{
    size_t got;
    return ParseRealArray(string, endPtr, array, count, count, &got) && got == count;
}

/// Decode a string that contains a series of floats. Skips whitespace before each number and skips whitespace and
/// an optional comma after every number except the last. This variant of the function requires that there be
/// nothing other than whitespace after the last number.
template <typename Real>
bool StringToRealArray(MaybeNullTerminatedStringView string, Real* array, size_t minCount, size_t maxCount,
    size_t* count) PRIME_NOEXCEPT
{
    const char* endPtr;
    if (!ParseRealArray(string, endPtr, array, minCount, maxCount, count)) {
        return false;
    }

    return ASCIISkipWhitespace(endPtr, string.end()) == string.end();
}

/// Decode a string that contains a series of floats. Skips whitespace before each number and skips whitespace and
/// an optional comma after every number except the last. This variant of the function requires that there be
/// nothing other than whitespace after the last number.
template <typename Real>
bool StringToRealArray(MaybeNullTerminatedStringView string, Real* array, size_t count) PRIME_NOEXCEPT
{
    const char* endPtr;
    if (!ParseRealArray(string, endPtr, array, count)) {
        return false;
    }

    return ASCIISkipWhitespace(endPtr, string.end()) == string.end();
}
}

#endif
