// Copyright 2000-2021 Mark H. P. Lord

#include "StringUtils.h"
#include "Optional.h"
#include <stdio.h>
#if defined(PRIME_STRTOMAX_USES_ERRNO) || defined(PRIME_STRTOFLOATMAX_USES_ERRNO)
#include <errno.h>
#endif
#include "Templates.h"
#include "TextEncoding.h"

namespace Prime {

const std::string emptyString;

const StringView asciiWhitespaceChars(" \t\r\n\f\v");

const StringView asciiNewlineChars("\r\n\f\v");

// https://en.wikipedia.org/wiki/Whitespace_character#Unicode
// u" \t\r\n\u000b\u000c\u0085\u00a0\u1680\u180e\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200a\u2028\u2029\u202f\u205f\u3000\ufeff".encode('UTF-8')
const StringView utf8WhitespaceChars(" \t\r\n\x0b\x0c\xc2\x85\xc2\xa0\xe1\x9a\x80\xe1\xa0\x8e\xe2\x80\x80\xe2\x80\x81\xe2\x80\x82\xe2\x80\x83\xe2\x80\x84\xe2\x80\x85\xe2\x80\x86\xe2\x80\x87\xe2\x80\x88\xe2\x80\x89\xe2\x80\x8a\xe2\x80\xa8\xe2\x80\xa9\xe2\x80\xaf\xe2\x81\x9f\xe3\x80\x80\xef\xbb\xbf");

// u"\r\n\u000b\u000c\u0085\u2028\u2029".encode('UTF-8')
const StringView utf8NewlineChars("\r\n\x0b\x0c\xc2\x85\xe2\x80\xa8\xe2\x80\xa9");

const StringView asciiAlphanumeric("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

//
// Utilities
//

size_t StringLength(const char* string, size_t maxLength)
{
    const char* ptr = string;
    const char* end = string + maxLength;

    while (ptr != end && *ptr) {
        ++ptr;
    }

    return (size_t)(ptr - string);
}

//
// FormatBuffer
//

namespace Private {

    /// Returns a string, allocated using operator new, containing the result of a vsprintf. There's no
    /// StringNewFormat function - use a FormatBuffer.
    static char* StringNewFormatLengthVA(size_t& length, const char* format, va_list argptr)
    {
#ifdef PRIME_GOOD_SNPRINTF

        // For C99 platforms, use the result of vsnprintf to guide us.

        va_list argptr2;
        PRIME_VA_COPY(argptr2, argptr);

        char tempBuffer[PRIME_BIG_STACK_BUFFER_SIZE];
        ptrdiff_t result1 = vsnprintf(tempBuffer, sizeof(tempBuffer), format, argptr2);

        va_end(argptr2);

        if (result1 < 0) {
            return NULL;
        }

        length = result1;

        if ((size_t)result1 < sizeof(tempBuffer)) {
            return NewString(tempBuffer, result1);
        }

        char* memory = new char[result1 + 1];
        if (!memory) {
            return NULL;
        }

        ptrdiff_t result2 = vsnprintf(memory, result1 + 1, format, argptr);

        if (!PRIME_GUARDMSG(result2 == result1, "Broken vsnprintf")) {
            delete[] memory;
            return NULL;
        }

        return memory;

#else

        // For non-C99 platforms, keep guessing.

        size_t bufferSize = PRIME_BIG_STACK_BUFFER_SIZE;
        for (;;) {
            bufferSize += bufferSize / 2;
            if (bufferSize > 32768u) {
                // Assume failure.
                return NULL;
            }

            char* buffer = new char[bufferSize];
            if (!buffer) {
                return NULL;
            }

            va_list argptr2;
            PRIME_VA_COPY(argptr2, argptr);

            size_t len;
            if (StringFormatLengthVA(len, buffer, bufferSize, format, argptr2)) {
                char* result;
                va_end(argptr2);
                result = NewString(buffer, len);
                length = len;
                delete[] buffer;
                return result;
            }

            va_end(argptr2);
            delete[] buffer;
        }

#endif
    }

    /// Returns the result of a vsprintf either in a supplied buffer, or in memory dynamically allocated with operator
    /// new. You must compare the return value to determine whether the buffer was used. There's no
    /// StringBufferNewFormat function - use a FormatBuffer.
    char* StringFormatAllocateOrUseBuffer(size_t& length, char* buffer, size_t bufferSize, const char* format, va_list argptr)
    {
        bool success;
        va_list argptr2;

        PRIME_VA_COPY(argptr2, argptr);

        success = StringFormatLengthVA(length, buffer, bufferSize, format, argptr2);

        va_end(argptr2);

        if (success) {
            return buffer;
        }

        return StringNewFormatLengthVA(length, format, argptr);
    }

}

//
// NewString
//

char* NewString(const char* string, size_t length)
{
    if (!string) {
        return 0;
    }

    char* copy = new char[length + 1];
    if (!copy) {
        return 0;
    }

    memcpy(copy, string, length);
    copy[length] = 0;
    return copy;
}

char* NewString(const char* string)
{
    if (!string) {
        return 0;
    }

    size_t len = strlen(string);
    size_t size = len + 1;

    char* copy = new char[size];
    if (!copy) {
        return 0;
    }

    memcpy(copy, string, size);
    return copy;
}

//
// CaseConverter
//

CaseConverter* CaseConverter::_global = NULL;

namespace {

    class ASCIICaseConverter : public CaseConverter {
    public:
        size_t toUpperCase(StringView source, char* dest, size_t destSize)
        {
            const char* ptr = source.begin();
            const char* end = source.end();
            char* destPtr = dest;
            char* destEnd = dest ? (dest + destSize) : dest;
            while (ptr != end) {
                if (destPtr < destEnd) {
                    *destPtr = ASCIIToUpper(*ptr);
                }
                ++ptr;
                ++destPtr;
            }

            return static_cast<size_t>(destPtr - dest);
        }

        size_t toLowerCase(StringView source, char* dest, size_t destSize)
        {
            const char* ptr = source.begin();
            const char* end = source.end();
            char* destPtr = dest;
            char* destEnd = dest ? (dest + destSize) : dest;
            while (ptr != end) {
                if (destPtr < destEnd) {
                    *destPtr = ASCIIToLower(*ptr);
                }
                ++ptr;
                ++destPtr;
            }

            return static_cast<size_t>(destPtr - dest);
        }

        size_t toTitleCase(StringView source, char* dest, size_t destSize)
        {
            const char* ptr = source.begin();
            const char* end = source.end();
            char* destPtr = dest;
            char* destEnd = dest ? (dest + destSize) : dest;

            for (;;) {
                while (ptr != end && ASCIIIsWhitespace(*ptr)) {
                    if (destPtr < destEnd) {
                        *destPtr = *ptr;
                    }
                    ++destPtr;
                    ++ptr;
                }

                if (ptr == end) {
                    break;
                }

                if (destPtr < destEnd) {
                    *destPtr = ASCIIToUpper(*ptr);
                }
                ++destPtr;
                ++ptr;

                while (ptr != end && !ASCIIIsWhitespace(*ptr)) {
                    if (destPtr < destEnd) {
                        *destPtr = *ptr;
                    }
                    ++destPtr;
                    ++ptr;
                }
            }

            return static_cast<size_t>(destPtr - dest);
        }

        size_t fold(StringView source, char* dest, size_t destSize)
        {
            return toLowerCase(source, dest, destSize);
        }
    };
}

CaseConverter* CaseConverter::getASCIIConverter()
{
    static ASCIICaseConverter asciiCaseConverter;
    return &asciiCaseConverter;
}

CaseConverter::~CaseConverter()
{
    if (_global == this) {
        _global = NULL;
    }
}

//
// ASCII
//

bool ContainsExtendedCharacters(StringView string)
{
    // Surprisingly important optimisations
    if (string.begin() == utf8WhitespaceChars.begin() && string.size() == utf8WhitespaceChars.size()) {
        return true;
    }
    if (string.begin() == asciiWhitespaceChars.begin() && string.size() == asciiWhitespaceChars.size()) {
        return false;
    }

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(string.begin());
    const uint8_t* end = reinterpret_cast<const uint8_t*>(string.end());

    for (; ptr != end; ++ptr) {
        if (*ptr & 0x80) {
            return true;
        }
    }

    return false;
}

void ASCIIToTitleCaseInPlace(const char* begin, const char* end, char* out)
{
    for (;;) {
        while (begin != end && ASCIIIsWhitespace(*begin)) {
            *out++ = *begin++;
        }

        if (begin == end) {
            break;
        }

        *out++ = ASCIIToUpper(*begin++);

        while (begin != end && !ASCIIIsWhitespace(*begin)) {
            *out++ = *begin++;
        }
    }
}

int ASCIICompareIgnoringCase(const char* a, const char* b)
{
    for (;;) {
        if (!*a) {
            return *b ? -1 : 0;
        }

        if (!*b) {
            return 1;
        }

        if (*a != *b) {
            unsigned char ca = static_cast<unsigned char>(ASCIIToLower(*a));
            unsigned char cb = static_cast<unsigned char>(ASCIIToLower(*b));

            if (ca < cb) {
                return -1;
            } else if (ca > cb) {
                return 1;
            }
        }

        ++a;
        ++b;
    }
}

int ASCIICompareIgnoringCase(const char* a, const char* b, size_t n)
{
    while (n--) {
        if (!*a) {
            return *b ? -1 : 0;
        }

        if (!*b) {
            return 1;
        }

        if (*a != *b) {
            unsigned char ca = static_cast<unsigned char>(ASCIIToLower(*a));
            unsigned char cb = static_cast<unsigned char>(ASCIIToLower(*b));

            if (ca < cb) {
                return -1;
            } else if (ca > cb) {
                return 1;
            }
        }

        ++a;
        ++b;
    }

    return 0;
}

int ASCIICompareIgnoringCase(const char* a, size_t aSize, const char* b, size_t bSize)
{
    size_t n = PRIME_MIN(aSize, bSize);
    for (; n; --n) {
        if (*a != *b) {
            unsigned char ca = static_cast<unsigned char>(ASCIIToLower(*a));
            unsigned char cb = static_cast<unsigned char>(ASCIIToLower(*b));

            if (ca < cb) {
                return -1;
            } else if (ca > cb) {
                return 1;
            }
        }

        ++a;
        ++b;
    }

    if (aSize == bSize) {
        return 0;
    }

    return aSize < bSize ? -1 : 1;
}

bool ASCIIEqualIgnoringCase(const char* a, size_t aSize, const char* b, size_t bSize)
{
    if (aSize != bSize) {
        return false;
    }

    while (aSize--) {
        if (ASCIIToLower(*a++) != ASCIIToLower(*b++)) {
            return false;
        }
    }

    return true;
}

void ASCIIToLowerInPlace(char* string)
{
    for (; *string; ++string) {
        *string = ASCIIToLower(*string);
    }
}

void ASCIIToUpperInPlace(char* string)
{
    for (; *string; ++string) {
        *string = ASCIIToUpper(*string);
    }
}

void ASCIIToTitleCaseInPlace(char* string)
{
    ASCIIToTitleCaseInPlace(string, string + StringLength(string), string);
}

bool ASCIIEqualIgnoringCase(const char* begin, const char* end, const char* other)
{
    while (begin != end) {
        if (*begin != *other) {
            if (ASCIIToLower(*begin) != ASCIIToLower(*other)) {
                return false;
            }
        }

        ++begin;
        ++other;
    }

    return true;
}

std::string ASCIIToLower(StringView string)
{
    std::string result;
    result.resize(string.size());
    std::transform(string.begin(), string.end(), result.begin(), static_cast<char (*)(int)>(ASCIIToLower));
    return result;
}

void ASCIIToLowerInPlace(std::string& string)
{
    std::transform(string.begin(), string.end(), string.begin(), static_cast<char (*)(int)>(ASCIIToLower));
}

std::string ASCIIToUpper(StringView string)
{
    std::string result;
    result.resize(string.size());
    std::transform(string.begin(), string.end(), result.begin(), static_cast<char (*)(int)>(ASCIIToUpper));
    return result;
}

void ASCIIToUpperInPlace(std::string& string)
{
    std::transform(string.begin(), string.end(), string.begin(), static_cast<char (*)(int)>(ASCIIToUpper));
}

std::string ASCIIToTitleCase(StringView string)
{
    std::string result;
    if (!string.empty()) {
        result.resize(string.size());
        ASCIIToTitleCaseInPlace(string.begin(), string.end(), &result[0]);
    }
    return result;
}

void ASCIIToTitleCaseInPlace(std::string& string)
{
    if (!string.empty()) {
        ASCIIToTitleCaseInPlace(&string[0], &string[0] + string.size(), &string[0]);
    }
}

void StringToLowerInPlace(std::string& string)
{
    std::string temp;
    temp.resize(CaseConverter::getGlobal()->toLowerCase(string, NULL, 0));
    CaseConverter::getGlobal()->toLowerCase(string, &temp[0], temp.size());
    string.swap(temp);
}

void StringToUpperInPlace(std::string& string)
{
    std::string temp;
    temp.resize(CaseConverter::getGlobal()->toUpperCase(string, NULL, 0));
    CaseConverter::getGlobal()->toUpperCase(string, &temp[0], temp.size());
    string.swap(temp);
}

void StringToTitleCaseInPlace(std::string& string)
{
    std::string temp;
    temp.resize(CaseConverter::getGlobal()->toTitleCase(string, NULL, 0));
    CaseConverter::getGlobal()->toTitleCase(string, &temp[0], temp.size());
    string.swap(temp);
}

void StringCaseFoldInPlace(std::string& string)
{
    std::string temp;
    temp.resize(CaseConverter::getGlobal()->fold(string, NULL, 0));
    CaseConverter::getGlobal()->fold(string, &temp[0], temp.size());
    string.swap(temp);
}

std::string StringToLower(StringView string)
{
    std::string result;
    result.resize(CaseConverter::getGlobal()->toLowerCase(string, NULL, 0));
    CaseConverter::getGlobal()->toLowerCase(string, &result[0], result.size());
    return result;
}

std::string StringToUpper(StringView string)
{
    std::string result;
    result.resize(CaseConverter::getGlobal()->toUpperCase(string, NULL, 0));
    CaseConverter::getGlobal()->toUpperCase(string, &result[0], result.size());
    return result;
}

std::string StringToTitleCase(StringView string)
{
    std::string result;
    result.resize(CaseConverter::getGlobal()->toTitleCase(string, NULL, 0));
    CaseConverter::getGlobal()->toTitleCase(string, &result[0], result.size());
    return result;
}

std::string StringCaseFold(StringView string)
{
    std::string result;
    result.resize(CaseConverter::getGlobal()->fold(string, NULL, 0));
    CaseConverter::getGlobal()->fold(string, &result[0], result.size());
    return result;
}

int StringCompareIngoringCase(StringView a, StringView b)
{
    size_t aSize = CaseConverter::getGlobal()->fold(a, NULL, 0);
    DynamicBuffer<char, 128> aBuffer(aSize);
    CaseConverter::getGlobal()->fold(a, aBuffer.get(), aBuffer.capacity());

    size_t bSize = CaseConverter::getGlobal()->fold(b, NULL, 0);
    DynamicBuffer<char, 128> bBuffer(bSize);
    CaseConverter::getGlobal()->fold(b, bBuffer.get(), bBuffer.capacity());

    int diff = memcmp(aBuffer.get(), bBuffer.get(), std::min(aSize, bSize));
    if (diff != 0) {
        return diff;
    }

    return aSize < bSize ? -1 : aSize > bSize ? 1 : 0;
}

bool StringsEqualIgnoringCase(StringView a, StringView b)
{
    size_t aSize = CaseConverter::getGlobal()->fold(a, NULL, 0);
    size_t bSize = CaseConverter::getGlobal()->fold(b, NULL, 0);
    if (aSize != bSize) {
        return false;
    }

    DynamicBuffer<char, 128> aBuffer(aSize);
    CaseConverter::getGlobal()->fold(a, aBuffer.get(), aBuffer.capacity());

    DynamicBuffer<char, 128> bBuffer(bSize);
    CaseConverter::getGlobal()->fold(b, bBuffer.get(), bBuffer.capacity());

    return memcmp(aBuffer.get(), bBuffer.get(), aSize) == 0;
}

//
// Wildcards
//

bool WildcardMatch(StringView wildcardView, StringView stringView, bool ignoreCase)
{
    const char* wildcard = wildcardView.begin();
    const char* string = stringView.begin();

    for (;;) {
        if (wildcard == wildcardView.end()) {
            return string == stringView.end();
        }

        if (*wildcard == '*') {
            ++wildcard;

            do {
                if (WildcardMatch(StringView(wildcard, wildcardView.end()),
                        StringView(string, stringView.end()),
                        ignoreCase)) {
                    return true;
                }
            } while (string++ != stringView.end());

            return false;
        }

        if (*wildcard == '?') {
            if (string == stringView.end()) {
                return false;
            }
        } else if (string == stringView.end()) {
            return false;
        } else if (*string != *wildcard) {
            if (!ignoreCase || ASCIIToLower(*string) != ASCIIToLower(*wildcard)) {
                return false;
            }
        }

        ++wildcard;
        ++string;
    }
}

bool WildcardMatch(StringView wildcardView, StringView stringView, bool ignoreCase, StringView separators)
{
    const char* wildcard = wildcardView.begin();
    const char* string = stringView.begin();

    for (;;) {
        if (wildcard == wildcardView.end()) {
            return string == stringView.end();
        }

        if (*wildcard == '*') {
            ++wildcard;

            if (wildcard != wildcardView.end() && *wildcard == '*') {
                ++wildcard;
                do {
                    if (WildcardMatch(StringView(wildcard, wildcardView.end()),
                            StringView(string, stringView.end()),
                            ignoreCase,
                            separators)) {
                        return true;
                    }
                } while (string++ != stringView.end());

            } else {
                do {
                    if (string != stringView.end() && Contains(separators, *string)) {
                        break;
                    }
                    if (WildcardMatch(StringView(wildcard, wildcardView.end()),
                            StringView(string, stringView.end()),
                            ignoreCase,
                            separators)) {
                        return true;
                    }
                } while (string++ != stringView.end());
            }

            return false;
        }

        if (*wildcard == '?') {
            if (string == stringView.end() || Contains(separators, *string)) {
                return false;
            }
        } else if (string == stringView.end()) {
            return false;
        } else if (*string != *wildcard) {
            if (!ignoreCase || ASCIIToLower(*string) != ASCIIToLower(*wildcard)) {
                return false;
            }
        }

        ++wildcard;
        ++string;
    }
}

//
// std::string formatting
//

std::string Format(const char* format, ...)
{
    std::string result;
    va_list argptr;
    va_start(argptr, format);
    StringAppendFormatVA(result, format, argptr);
    va_end(argptr);
    return result;
}

void StringFormatVA(std::string& target, const char* format, va_list argptr)
{
    target.resize(0);
    StringAppendFormatVA(target, format, argptr);
}

void StringFormat(std::string& target, const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    StringFormatVA(target, format, argptr);
    va_end(argptr);
}

void StringAppendFormatVA(std::string& target, const char* format, va_list argptr)
{
    size_t sizeWas = target.size();

    if (target.capacity() - sizeWas < 8) {
        target.reserve(sizeWas + 32);
    }

    // I reserve an extra character for vsnprintf to write its '\0' because there's no guarantee that the
    // underlying std::string implementation reserves space for a '\0'.

    target.resize(target.capacity() - 1);

#ifdef PRIME_GOOD_SNPRINTF

    // For C99 platforms, use the result of vsnprintf to guide us.

    va_list argptr2;
    PRIME_VA_COPY(argptr2, argptr);

    ptrdiff_t result1 = vsnprintf(&target[sizeWas], target.size() - sizeWas + 1, format, argptr2);

    va_end(argptr2);

    if (result1 < 0) {
        target.resize(0);
        return;
    }

    if ((size_t)result1 < target.size() - sizeWas + 1) {
        target.resize(result1 + sizeWas);
        return;
    }

    target.reserve(result1 + sizeWas + 1);
    target.resize(result1 + sizeWas);

    ptrdiff_t result2 = vsnprintf(&target[sizeWas], target.size() - sizeWas + 1, format, argptr);

    if (!PRIME_GUARDMSG(result2 == result1, "Broken vsnprintf")) {
        target.resize(0);
        return;
    }

#else

    // For non-C99 platforms, keep guessing.

    for (;;) {
        va_list argptr2;
        PRIME_VA_COPY(argptr2, argptr);

        size_t len;
        if (StringFormatLengthVA(len, &target[sizeWas], target.size() - sizeWas, format, argptr2)) {
            va_end(argptr2);
            target.resize(len + sizeWas);
            return;
        }

        va_end(argptr2);

        target.resize(target.size() * 2);
    }

#endif
}

void StringAppendFormat(std::string& target, const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    StringAppendFormatVA(target, format, argptr);
    va_end(argptr);
}

bool StringCopy(char* buffer, size_t bufferSize, const std::string& string)
{
    return StringCopy(buffer, bufferSize, string, string.size());
}

bool StringCopy(char* buffer, size_t bufferSize, const std::string& string, size_t n)
{
    if (!PRIME_GUARD(bufferSize)) {
        return false;
    }

    if (n > string.size()) {
        n = string.size();
    }

    bool ok = true;

    if (n > bufferSize - 1) {
        n = bufferSize - 1;
        ok = false;
    }

    string.copy(buffer, n);
    buffer[n] = '\0';
    return ok;
}

bool StringAppend(char* buffer, size_t bufferSize, const std::string& string)
{
    return StringAppend(buffer, bufferSize, string, string.size());
}

bool StringAppend(char* buffer, size_t bufferSize, const std::string& string, size_t n)
{
    if (n > string.size()) {
        n = string.size();
    }

    if (n >= bufferSize) {
        return false;
    }

    return StringCopy(buffer + n, bufferSize - n, string, n);
}

//
// String Parsing
//

//
// Parsing
//

const char* ASCIISkipSpacesAndTabs(const char* string, const char* end) PRIME_NOEXCEPT
{
    while (string != end && (*string == ' ' || *string == '\t')) {
        ++string;
    }

    return string;
}

const char* ASCIISkipSpacesAndTabs(const char* string) PRIME_NOEXCEPT
{
    while (*string == ' ' || *string == '\t') {
        ++string;
    }

    return string;
}

const char* ASCIIReverseSkipSpacesAndTabs(const char* begin, const char* ptr) PRIME_NOEXCEPT
{
    while (ptr > begin && ASCIIIsSpaceOrTab(ptr[-1])) {
        --ptr;
    }

    return ptr;
}

const char* ASCIISkipWhitespace(const char* string, const char* end) PRIME_NOEXCEPT
{
    while (string != end && *string <= ' ' && *string >= 0) {
        ++string;
    }

    return string;
}

const char* ASCIISkipWhitespace(const char* string) PRIME_NOEXCEPT
{
    while (*string > 0 && *string <= ' ') {
        ++string;
    }

    return string;
}

const char* ASCIISkipSpacesTabsAndNewlines(const char* string, const char* end) PRIME_NOEXCEPT
{
    while (string != end && ASCIIIsSpaceOrTabOrNewline(*string)) {
        ++string;
    }

    return string;
}

const char* ASCIISkipSpacesTabsAndNewlines(const char* string) PRIME_NOEXCEPT
{
    while (ASCIIIsSpaceOrTabOrNewline(*string)) {
        ++string;
    }

    return string;
}

const char* ASCIISkipUntilNewline(const char* string, const char* end) PRIME_NOEXCEPT
{
    while (string != end && !ASCIIIsNewline(*string)) {
        ++string;
    }

    return string;
}

const char* ASCIISkipUntilNewline(const char* string) PRIME_NOEXCEPT
{
    while (*string && !ASCIIIsNewline(*string)) {
        ++string;
    }

    return string;
}

const char* ASCIISkipNewline(const char* string, const char* end) PRIME_NOEXCEPT
{
    if (string == end) {
        return string;
    }

    if (*string == '\r') {
        if (string + 1 != end && string[1] == '\n') {
            ++string;
        }
    } else {
        PRIME_DEBUG_ASSERTMSG(*string == '\n', "ASCIISkipNewline expects *string to point to a newline");
        if (string + 1 != end && string[1] == '\r') {
            ++string;
        }
    }

    ++string;
    return string;
}

const char* ASCIISkipNewline(const char* string) PRIME_NOEXCEPT
{
    if (!*string) {
        return string;
    }

    if (*string == '\r') {
        if (string[1] == '\n') {
            ++string;
        }
    } else {
        PRIME_DEBUG_ASSERTMSG(*string == '\n', "ASCIISkipNewline expects *string to point to a newline");
        if (string[1] == '\r') {
            ++string;
        }
    }

    ++string;
    return string;
}

const char* ASCIISkipNextNewline(const char* string, const char* end) PRIME_NOEXCEPT
{
    return ASCIISkipNewline(ASCIISkipUntilNewline(string, end), end);
}

const char* ASCIISkipNextNewline(const char* string) PRIME_NOEXCEPT
{
    return ASCIISkipNewline(ASCIISkipUntilNewline(string));
}

const char* ASCIISkipUntilSpaceOrTab(const char* ptr, const char* end) PRIME_NOEXCEPT
{
    while (ptr != end && *ptr != ' ' && *ptr != '\t') {
        ++ptr;
    }

    return ptr;
}

const char* ASCIISkipUntilSpaceOrTab(const char* ptr) PRIME_NOEXCEPT
{
    while (*ptr && *ptr != ' ' && *ptr != '\t') {
        ++ptr;
    }

    return ptr;
}

const char* ASCIISkipWhitespaceForArray(const char* string, const char* end, char separator) PRIME_NOEXCEPT
{
    string = ASCIISkipWhitespace(string, end);
    if (separator && string != end && *string == separator) {
        string = ASCIISkipWhitespace(string + 1, end);
    }

    return string;
}

const char* ASCIISkipWhitespaceForArray(const char* string, char separator) PRIME_NOEXCEPT
{
    string = ASCIISkipWhitespace(string);
    if (*string == separator && separator) {
        string = ASCIISkipWhitespace(string + 1);
    }

    return string;
}

const char* ASCIISkipChar(const char* string, const char* end, char ch) PRIME_NOEXCEPT
{
    while (string != end && *string == ch) {
        ++string;
    }

    return string;
}

const char* ASCIISkipChar(const char* string, char ch) PRIME_NOEXCEPT
{
    while (*string == ch) {
        ++string;
    }

    return string;
}

const char* ASCIISkipChars(const char* string, const char* end, StringView chars) PRIME_NOEXCEPT
{
    while (string != end && chars.find(*string) != StringView::npos) {
        ++string;
    }

    return string;
}

const char* ASCIISkipChars(const char* string, StringView chars) PRIME_NOEXCEPT
{
    while (*string && chars.find(*string) != StringView::npos) {
        ++string;
    }

    return string;
}

const char* StringLastComponent(const char* string, StringView separators, UTF8Mode utf8Mode) PRIME_NOEXCEPT
{
    return StringLastComponent(string, string + strlen(string), separators, utf8Mode);
}

const char* StringLastComponent(const char* string, const char* end, StringView separators, UTF8Mode utf8Mode) PRIME_NOEXCEPT
{
    if (utf8Mode == UTF8ModeUnknown) {
        utf8Mode = ContainsExtendedCharacters(separators) ? UTF8ModeUTF8 : UTF8ModeASCII;
    }

    if (utf8Mode == UTF8ModeUTF8) {

        StringView view(string, end);

        StringView::size_type index = UTF8FindLastOf(view, separators);
        if (index == StringView::npos) {
            return string;
        }

        return reinterpret_cast<const char*>(UTF8Advance(view.begin() + index, view.end(), 1));

    } else {

        StringView view(string, end);

        StringView::size_type index = view.find_last_of(separators);
        if (index == StringView::npos) {
            return string;
        }

        return view.begin() + index + 1;
    }
}

//
// TokenParser
//

namespace {

    /// Old C API - this is the guts of TokenParser but hidden for embarrassment reasons
    bool ParseToken(const char*& string, const char* stringEnd, const char*& tokenBegin, const char*& tokenEnd,
        char& sep, const char* separators, unsigned int options) PRIME_NOEXCEPT
    {
        if (!separators) {
            separators = "";
        }

        const char* ptr = string;

        const bool backslashIsEscape = (options & TokenParser::OptionBackslashIsEscape) ? true : false;

        ptr = ASCIISkipWhitespace(ptr, stringEnd);
        if (ptr == stringEnd) {
            string = tokenBegin = tokenEnd = stringEnd;
            return false;
        }

        if (*ptr == '"') {
            // Quoted value.
            char quote = *ptr++;
            tokenBegin = ptr;

            while (ptr != stringEnd && *ptr != quote) {
                if (*ptr == '\\' && backslashIsEscape) {
                    if (ptr + 1 != stringEnd) {
                        ++ptr;
                    }
                }

                ++ptr;
            }

            tokenEnd = ptr;
            if (ptr != stringEnd) {
                ++ptr;
            }

        } else {
            tokenBegin = ptr;

            // Non-quoted value.
            while (ptr != stringEnd && !ASCIIIsWhitespace(*ptr) && !strchr(separators, *ptr)) {
                if (*ptr == '\\' && backslashIsEscape) {
                    if (ptr + 1 != stringEnd) {
                        ++ptr;
                    }
                }

                ++ptr;
            }

            tokenEnd = ptr;
        }

        // We need to seek ahead and look for separators.

        ptr = ASCIISkipWhitespace(ptr, stringEnd);

        if (ptr != stringEnd && strchr(separators, *ptr)) {
            sep = *ptr++;
        } else {
            sep = 0;
        }

        string = ptr;
        return true;
    }
}

TokenParser::TokenParser()
{
    construct();
}

TokenParser::TokenParser(StringView string)
    : _string(string)
{
    construct();
}

void TokenParser::construct()
{
    _separator = 0;
    _ok = false;
}

void TokenParser::init(StringView string)
{
    construct();
    _string = string;
}

bool TokenParser::parse(StringView& token, const char* separators, unsigned int options)
{
    const char* tokenBegin;
    const char* tokenEnd;
    const char* stringBegin = _string.begin();
    _ok = ParseToken(stringBegin, _string.end(), tokenBegin, tokenEnd, _separator, separators, options);
    _stringWas = _string;
    _string = StringView(stringBegin, _string.end());
    _token = token = StringView(tokenBegin, tokenEnd);
    return _ok;
}

bool TokenParser::parse(ArrayView<char> tokenBuffer, const char* separators, unsigned int options)
{
    parse(_token, separators, options);
    return StringCopy(tokenBuffer, _token) && _ok;
}

//
// String Trimming
//

StringViewPair StringViewBisectLine(StringView view) PRIME_NOEXCEPT
{
    const char* newline = ASCIISkipUntilNewline(view.begin(), view.end());
    return std::make_pair(StringView(view.begin(), newline),
        StringView(ASCIISkipNewline(newline, view.end()), view.end()));
}

void StringRightTrimInPlace(std::string& string, StringView chars, UTF8Mode utf8Mode)
{
    string.resize(StringViewRightTrim(string, chars, utf8Mode).size());
}

void StringLeftTrimInPlace(std::string& string, StringView chars, UTF8Mode utf8Mode)
{
    string.erase(0, string.size() - StringViewLeftTrim(string, chars, utf8Mode).size());
}

void StringTrimInPlace(std::string& string, StringView chars, UTF8Mode utf8Mode)
{
    StringLeftTrimInPlace(string, chars, utf8Mode);
    StringRightTrimInPlace(string, chars, utf8Mode);
}

StringView StringViewRightTrim(StringView string, StringView chars, UTF8Mode utf8Mode)
{
    if (utf8Mode == UTF8ModeUnknown) {
        utf8Mode = ContainsExtendedCharacters(chars) ? UTF8ModeUTF8 : UTF8ModeASCII;
    }
    StringView::size_type i = (utf8Mode == UTF8ModeUTF8) ? UTF8FindLastNotOf(string, chars) : string.find_last_not_of(chars);
    return i == StringView::npos ? StringView() : string.substr(0, i + 1);
}

StringView StringViewLeftTrim(StringView string, StringView chars, UTF8Mode utf8Mode)
{
    if (utf8Mode == UTF8ModeUnknown) {
        utf8Mode = ContainsExtendedCharacters(chars) ? UTF8ModeUTF8 : UTF8ModeASCII;
    }
    return string.substr((utf8Mode == UTF8ModeUTF8) ? UTF8FindFirstNotOf(string, chars) : string.find_first_not_of(chars));
}

StringView StringViewTrim(StringView string, StringView chars, UTF8Mode utf8Mode)
{
    if (utf8Mode == UTF8ModeUnknown) {
        // We do this test to avoid it being done twice
        utf8Mode = ContainsExtendedCharacters(chars) ? UTF8ModeUTF8 : UTF8ModeASCII;
    }
    return StringViewRightTrim(StringViewLeftTrim(string, chars, utf8Mode), chars, utf8Mode);
}

bool StringIsWhitespace(StringView string, StringView whitespace, UTF8Mode utf8Mode)
{
    return StringViewTrim(string, whitespace, utf8Mode).empty();
}

void MiddleTruncateStringInPlace(std::string& string, size_t maxSize, const std::string& ellipsis)
{
    if (string.size() <= maxSize) {
        return;
    }

    size_t over = string.size() - maxSize;
    string.erase(maxSize / 2, over);
    if (maxSize > ellipsis.size() * 3) {
        string.replace(maxSize / 2 - ellipsis.size() / 2, ellipsis.size(), ellipsis);
    }
}

//
// String replace
//

void StringReplaceInPlace(std::string& string, StringView from, StringView to, size_t startSearch)
{
    size_t offset = startSearch;
    while ((offset = string.find(from.begin(), offset, from.size())) != std::string::npos) {
        string.replace(offset, from.size(), to.begin(), to.size());
        offset += to.size();
    }
}

std::string StringReplace(StringView string, StringView from, StringView to, size_t startSearch)
{
    std::string result(string.data(), string.size());
    StringReplaceInPlace(result, from, to, startSearch);
    return result;
}

bool StringReplaceInPlace(char* buffer, size_t bufferSize, StringView source, StringView replaceThis,
    StringView withThis)
{
    if (!PRIME_GUARD(bufferSize)) {
        return false;
    }

    if (replaceThis.empty()) {
        return StringCopy(buffer, bufferSize, source);
    }

    const char firstChar = replaceThis[0];

    const char* ptr = source.begin();
    const char* end = source.end();
    for (;;) {
        PRIME_ASSERT(bufferSize);
        const char* begin = ptr;
        for (; ptr != end; ++ptr) {
            if (*ptr == firstChar && (size_t)(end - ptr) >= replaceThis.size()) {
                if (memcmp(ptr, replaceThis.begin(), replaceThis.size()) == 0) {
                    break;
                }
            }
        }

        size_t length = (size_t)(ptr - begin);
        if (length >= bufferSize) {
            return false;
        }

        memcpy(buffer, begin, length);

        bufferSize -= length;
        buffer += length;

        if (ptr == end) {
            break;
        }

        ptr += replaceThis.size();

        if (withThis.size() >= bufferSize) {
            return false;
        }

        memcpy(buffer, withThis.begin(), withThis.size());
        buffer += withThis.size();
        bufferSize -= withThis.size();
    }

    PRIME_ASSERT(bufferSize);
    *buffer = '\0';
    return true;
}

StringView::size_type StringFind(StringView string, StringView findThis, size_t startSearch)
{
    return string.find(findThis, startSearch);
}

StringView::size_type ASCIIFindIgnoringCase(StringView source, StringView findThis, size_t startSearch)
{
    if (findThis.empty() || startSearch >= source.size()) {
        return StringView::npos;
    }

    const char firstCharLower = ASCIIToLower(findThis[0]);
    const char firstCharUpper = ASCIIToUpper(findThis[0]);

    const char* ptr = source.begin() + startSearch;
    const char* end = source.end();

    for (; ptr != end; ++ptr) {
        if ((*ptr == firstCharLower || *ptr == firstCharUpper) && (size_t)(end - ptr) >= findThis.size()) {
            if (ASCIICompareIgnoringCase(ptr, findThis.begin(), findThis.size()) == 0) {
                return static_cast<StringView::size_type>(ptr - source.begin());
            }
        }
    }
    return StringView::npos;
}

/// Reads the first substring of the form [+-]\d+
std::string StringExtractNumber(StringView string)
{
    const char* begin = string.begin();
    const char* end = string.end();
    const char* ptr = begin;

    std::string result;

    while (ptr != end) {
        if (ASCIIIsDigit(*ptr)) {
            const char* numberBegin = ptr;
            bool hasDecimal = false;

            if (numberBegin != begin && (numberBegin[-1] == '.')) {
                --numberBegin;
                hasDecimal = true;
            }
            if (numberBegin != begin && (numberBegin[-1] == '-' || numberBegin[-1] == '+')) {
                --numberBegin;
            }

            do {
                ++ptr;
            } while (ptr != end && ASCIIIsDigit(*ptr));

            if (!hasDecimal && *ptr == '.') {
                do {
                    ++ptr;
                } while (ptr != end && ASCIIIsDigit(*ptr));
            }

            result.assign(numberBegin, ptr);
            break;
        }

        ++ptr;
    }

    return result;
}

std::string ASCIIOnlyNumeric(StringView string)
{
    const char* ptr = string.begin();
    const char* end = string.end();

    std::string output;
    output.reserve(string.size());

    for (; ptr != end; ++ptr) {
        if (ASCIIIsDigit(*ptr)) {
            output += *ptr;
        }
    }

    return output;
}

std::string ASCIIOnlyAlphanumeric(StringView string)
{
    const char* ptr = string.begin();
    const char* end = string.end();

    std::string output;
    output.reserve(string.size());

    for (; ptr != end; ++ptr) {
        if (ASCIIIsAlphanumeric(*ptr)) {
            output += *ptr;
        }
    }

    return output;
}

std::string ASCIIOnlyAlphanumericUppercase(StringView string)
{
    const char* ptr = string.begin();
    const char* end = string.end();

    std::string output;
    output.reserve(string.size());

    for (; ptr != end; ++ptr) {
        if (ASCIIIsAlphanumeric(*ptr)) {
            output += ASCIIToUpper(*ptr);
        }
    }

    return output;
}

//
// String splitting
//

StringViewPair StringViewBisect(StringView string, char ch)
{
    StringView::size_type i = string.find(ch);
    return (i == StringView::npos) ? std::make_pair(string, StringView())
                                   : std::make_pair(string.substr(0, i), string.substr(i + 1));
}

StringViewPair StringViewBisect(StringView string, StringView separator)
{
    StringView::size_type i = string.find(separator);
    return (i == StringView::npos) ? std::make_pair(string, StringView())
                                   : std::make_pair(string.substr(0, i), string.substr(i + separator.size()));
}

StringViewPair StringViewBisectOnSeparators(StringView string, StringView separators, UTF8Mode utf8Mode)
{
    if (utf8Mode == UTF8ModeUnknown) {
        utf8Mode = ContainsExtendedCharacters(separators) ? UTF8ModeUTF8 : UTF8ModeASCII;
    }
    StringView::size_type i = (utf8Mode == UTF8ModeUTF8) ? UTF8FindFirstOf(string, separators) : string.find_first_of(separators);
    return (i == StringView::npos) ? std::make_pair(string, StringView())
                                   : std::make_pair(string.substr(0, i), UTF8Advance(string.substr(i), 1));
}

StringViewPair StringViewReverseBisect(StringView string, char ch)
{
    StringView::size_type i = string.rfind(ch);
    return (i == StringView::npos) ? std::make_pair(StringView(), string)
                                   : std::make_pair(string.substr(0, i), string.substr(i + 1));
}

StringViewPair StringViewReverseBisect(StringView string, StringView separator)
{
    StringView::size_type i = string.rfind(separator);
    return (i == StringView::npos) ? std::make_pair(StringView(), string)
                                   : std::make_pair(string.substr(0, i), string.substr(i + separator.size()));
}

StringViewPair StringViewReverseBisectOnSeparators(StringView string, StringView separators, UTF8Mode utf8Mode)
{
    if (utf8Mode == UTF8ModeUnknown) {
        utf8Mode = ContainsExtendedCharacters(separators) ? UTF8ModeUTF8 : UTF8ModeASCII;
    }
    StringView::size_type i = (utf8Mode == UTF8ModeUTF8) ? UTF8FindLastOf(string, separators) : string.find_last_of(separators);
    return (i == StringView::npos) ? std::make_pair(StringView(), string)
                                   : std::make_pair(string.substr(0, i), UTF8Advance(string.substr(i), 1));
}

void StringSplit(std::vector<std::string>& vector, StringView string, StringView separator, unsigned int flags)
{
    PRIME_ASSERT(!separator.empty());
    for (size_t offset = 0;;) {
        size_t next = string.find(separator, offset);

        StringView piece = string.substr(offset, next - offset);

        if (!(flags & SplitKeepWhitespace)) {
            piece = StringViewTrim(piece);
        }

        if (!(flags & SplitSkipEmpty) || !piece.empty()) {
#ifdef PRIME_CXX11_STL
            vector.emplace_back(piece.begin(), piece.end());
#else
            vector.push_back(piece.to_string());
#endif
        }

        if (next == StringView::npos) {
            break;
        }

        offset = next + separator.size();
    }
}

std::vector<std::string> StringSplit(StringView string, StringView separator, unsigned int flags)
{
    std::vector<std::string> vector;
    StringSplit(vector, string, separator, flags);
    return vector;
}

void StringSplit(std::vector<StringView>& vector, StringView string, StringView separator, unsigned int flags)
{
    PRIME_ASSERT(!separator.empty());
    for (size_t offset = 0;;) {
        size_t next = string.find(separator, offset);

        StringView piece = string.substr(offset, next - offset);

        if (!(flags & SplitKeepWhitespace)) {
            piece = StringViewTrim(piece);
        }

        if (!(flags & SplitSkipEmpty) || !piece.empty()) {
            vector.push_back(piece);
        }

        if (next == StringView::npos) {
            break;
        }

        offset = next + separator.size();
    }
}

std::vector<StringView> StringViewSplit(StringView string, StringView separator, unsigned int flags)
{
    std::vector<StringView> vector;
    StringSplit(vector, string, separator, flags);
    return vector;
}

void StringSplitOnSeparators(std::vector<StringView>& vector, StringView string, StringView separators,
    unsigned int flags, UTF8Mode utf8Mode)
{
    PRIME_ASSERT(!separators.empty());

    if (utf8Mode == UTF8ModeUnknown) {
        utf8Mode = ContainsExtendedCharacters(separators) ? UTF8ModeUTF8 : UTF8ModeASCII;
    }

    if (utf8Mode == UTF8ModeUTF8) {

        for (;;) {
            size_t next = UTF8FindFirstOf(string, separators);

            StringView piece = string.substr(0, next);

            if (!(flags & SplitKeepWhitespace)) {
                piece = StringViewTrim(piece);
            }

            if (!(flags & SplitSkipEmpty) || !piece.empty()) {
                vector.push_back(piece);
            }

            if (next == StringView::npos) {
                break;
            }

            string = UTF8Advance(string.substr(next), 1);
        }

    } else {

        for (size_t offset = 0;;) {
            size_t next = string.find_first_of(separators, offset);

            StringView piece = string.substr(offset, next - offset);

            if (!(flags & SplitKeepWhitespace)) {
                piece = StringViewTrim(piece);
            }

            if (!(flags & SplitSkipEmpty) || !piece.empty()) {
                vector.push_back(piece);
            }

            if (next == StringView::npos) {
                break;
            }

            offset = next + 1;
        }
    }
}

std::vector<StringView> SplitSeparatorsViews(StringView string, StringView separators,
    unsigned int flags, UTF8Mode utf8Mode)
{
    std::vector<StringView> vector;
    StringSplitOnSeparators(vector, string, separators, flags, utf8Mode);
    return vector;
}

void StringSplitOnSeparators(std::vector<std::string>& vector, StringView string, StringView separators,
    unsigned int flags, UTF8Mode utf8Mode)
{
    std::vector<StringView> views;
    StringSplitOnSeparators(views, string, separators, flags, utf8Mode);

    for (std::vector<StringView>::const_iterator iter = views.begin(); iter != views.end(); ++iter) {
        vector.push_back(iter->to_string());
    }
}

std::vector<std::string> StringSplitOnSeparators(StringView string, StringView separators, unsigned int flags,
    UTF8Mode utf8Mode)
{
    std::vector<std::string> vector;
    StringSplitOnSeparators(vector, string, separators, flags, utf8Mode);
    return vector;
}

std::vector<StringView> StringViewSplitLines(StringView string)
{
    return SplitSeparatorsViews(string, utf8NewlineChars);
}

std::vector<std::string> StringSplitLines(StringView string)
{
    return StringSplitOnSeparators(string, utf8NewlineChars);
}

std::string StringRepeat(StringView string, size_t count)
{
    std::string result;

    if (count) {
        result.reserve(count * string.size());

        while (count--) {
            result += string;
        }
    }

    return result;
}

//
// String substitution
//

VariableExpander::~VariableExpander()
{
}

std::string StringExpandDollarVariables(StringView string, const VariableExpander& expander)
{
    std::string target;
    StringExpandDollarVariablesInPlace(target, string, expander);
    return target;
}

void StringExpandDollarVariablesInPlace(std::string& target, StringView string, const VariableExpander& expander)
{
    static const char braces[] = "(){}[]";
    const size_t maxVariableName = 128;
    char variableName[maxVariableName];
    const char* begin = string.begin();
    const char* end = string.end();

    for (const char* ptr = begin;;) {
        const char* start = ptr;

        while (ptr != end && *ptr != '$') {
            ++ptr;
        }

        target.append(start, ptr);

        if (ptr == end) {
            break;
        }

        PRIME_ASSERT(*ptr == '$');

        ++ptr;
        if (ptr == end) {
            // Bad input. Ignore the $.
            break;
        }

        if (*ptr == '$') {
            // Special case $$.
            static const char dollar[] = "$";
            target.append(dollar, dollar + 1);

            ++ptr;
            continue;
        }

        const char* brace = strchr(braces, *ptr);

        // TODO: support nested braces?
        if (brace && !((brace - braces) & 1)) {
            ++ptr;
            start = ptr;

            while (ptr != end && *ptr != brace[1]) {
                ++ptr;
            }

            StringCopy(variableName, sizeof(variableName), start, (size_t)(ptr - start));

            expander.appendVariable(target, variableName, brace[0]);

            if (ptr != end) {
                ++ptr;
            }

            continue;
        }

        if (ASCIIIsIdentifier(*ptr)) {
            start = ptr;

            while (ptr != end && ASCIIIsIdentifier(*ptr)) {
                ++ptr;
            }

            StringCopy(variableName, sizeof(variableName), start, (size_t)(ptr - start));

            expander.appendVariable(target, variableName, 0);

            continue;
        }

        variableName[0] = *ptr;
        variableName[1] = '\0';
        expander.appendVariable(target, variableName, 0);

        ++ptr;
    }
}

//
// Fuzzy comparisons
//

bool HybridIsWordChar::operator()(const char* ptr, const char* end, const char*& next)
{
    // Skip to first non-whitespace character
    size_t firstWordChar = UTF8FindFirstNotOf(StringView(ptr, end), utf8WhitespaceChars);
    if (firstWordChar == 0) {
        // First character is not whitespace. See if it's ASCII ignorable.
        const char* ptrWas = ptr;
        while (ptr != end && (*reinterpret_cast<const unsigned char*>(ptr) & 0x80) == 0 && !ASCIIIsAlphanumeric(*ptr)) {
            ++ptr;
        }
        if (ptr != ptrWas) {
            // We skipped some non-alphanumeric ASCII.
            next = ptr;
            return false;
        }

        // We didn't skip anything, so we have UNICODE or non-alphanumeric ASCII.
        next = UTF8Advance(ptr, end, 1);
        return true;
    }

    // It's whitespace all the way to the end.
    if (firstWordChar == StringView::npos) {
        next = end;
        return false;
    }

    // We skipped some ASCII whitespace. See if we can keep skipping.
    ptr += firstWordChar;
    while (ptr != end && (*reinterpret_cast<const unsigned char*>(ptr) & 0x80) == 0 && !ASCIIIsAlphanumeric(*ptr)) {
        ++ptr;
    }

    next = ptr;
    return false;
}

template <typename WordParserType>
static inline std::vector<std::string> ToWordsWithWordParser(StringView string, const std::set<std::string>& stopwords)
{
    WordParserType parser(string);
    std::vector<std::string> words;

    while (Optional<StringView> word = parser.next()) {
        bool stop = false;
        for (std::set<std::string>::const_iterator iter = stopwords.begin(); iter != stopwords.end(); ++iter) {
            if (StringsEqualIgnoringCase(*word, *iter)) {
                stop = true;
                break;
            }
        }

        if (stop) {
            continue;
        }

        words.push_back(word->to_string());
    }

    return words;
}

std::vector<std::string> ASCIIToWords(StringView string, const std::set<std::string>& stopwords)
{
    return ToWordsWithWordParser<ASCIIWordParser>(string, stopwords);
}

std::vector<std::string> HybridToWords(StringView string, const std::set<std::string>& stopwords)
{
    return ToWordsWithWordParser<HybridWordParser>(string, stopwords);
}

static inline bool ASCIIIsDigits(StringView a)
{
    StringView::const_iterator i = a.begin(), e = a.end();
    for (; i != e; ++i) {
        if (!ASCIIIsDigit(*i)) {
            return false;
        }
    }

    return true;
}

template <typename WordParser, typename CaseEqualFunc>
bool CloseEnough2(StringView a, StringView b, CaseEqualFunc caseEqual)
{
    WordParser ap(a);
    WordParser bp(b);

    Optional<StringView> aword;
    Optional<StringView> bword;
    for (;;) {
        if (!aword) {
            aword = ap.next();
        }
        if (!bword) {
            bword = bp.next();
        }

        for (;;) {
            if (!aword) {
                return !bword;
            } else if (!bword) {
                return false;
            }

            if (*aword == *bword) {
                aword = nullopt;
                bword = nullopt;
                break;
            }

            if (caseEqual(*aword, *bword)) {
                aword = nullopt;
                bword = nullopt;
                break;
            }

            if (ASCIIIsDigits(*aword) && ASCIIIsDigits(*bword)) {
                size_t minLength = std::min(aword->size(), bword->size());
                if (aword->substr(0, minLength) == bword->substr(0, minLength)) {
                    *aword = aword->substr(minLength);
                    *bword = bword->substr(minLength);
                    if (aword->empty()) {
                        aword = nullopt;
                    }
                    if (bword->empty()) {
                        bword = nullopt;
                    }
                    break;
                }
            }

            return false;
        }
    }
#if 0
        const char* aPtr = a.begin();
        const char* aEnd = a.end();
        const char* bPtr = b.begin();
        const char* bEnd = b.end();
        
        for (;;) {
            while (aPtr != aEnd && ! ASCIIIsAlphanumeric(*aPtr)) {
                ++aPtr;
            }
            
            while (bPtr != bEnd && ! ASCIIIsAlphanumeric(*bPtr)) {
                ++bPtr;
            }
            
            if (aPtr == aEnd) {
                return bPtr == bEnd;
            }
            
            bool matched = false;
            for (;;) {
                char ac = *aPtr++;
                char bc = *bPtr++;
                
                if (ASCIIToLower(ac) == ASCIIToLower(bc)) {
                    if (aPtr == aEnd || bPtr == bEnd) {
                        break;
                    }
                    
                    matched = true;
                    continue;
                }
                
                --aPtr;
                --bPtr;
                break;
            }
            
            if (! matched) {
                return false;
            }
        }
#endif
}

static bool CloseEnoughASCIICaseEqual(StringView a, StringView b)
{
    return ASCIIEqualIgnoringCase(a, b);
}

bool StringsEqualIgnoringNonAlphanumeric(StringView a, StringView b)
{
    if (ContainsExtendedCharacters(a) || ContainsExtendedCharacters(b)) {
        return CloseEnough2<HybridWordParser>(StringCaseFold(a), StringCaseFold(b), StringsEqualIgnoringCase);
    } else {
        return CloseEnough2<ASCIIWordParser>(a, b, CloseEnoughASCIICaseEqual);
    }
}

bool ASCIIContainsNonAlphanumeric(StringView string, UTF8Mode utf8Mode)
{
    if (utf8Mode == UTF8ModeUnknown) {
        utf8Mode = ContainsExtendedCharacters(string) ? UTF8ModeUTF8 : UTF8ModeASCII;
    }

    if (utf8Mode == UTF8ModeASCII) {
        const char* ptr = string.begin();
        const char* end = string.end();
        for (; ptr != end; ++ptr) {
            if (ASCIIIsAlphanumeric(*ptr)) {
                return false;
            }
        }

        return true;
    }

    return UTF8FindFirstOf(string, asciiAlphanumeric) != StringView::npos;
}

std::string StringToInitials(StringView a)
{
    HybridWordParser wp(a);

    std::string output;

    for (;;) {
        Optional<StringView> word = wp.next();
        if (!word) {
            break;
        }

        const char* next = UTF8Advance(word->begin(), word->end(), 1);

        output += StringToUpper(StringView(word->begin(), next));
    }

    return output;
}
}
