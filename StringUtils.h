// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STRINGUTILS_H
#define PRIME_STRINGUTILS_H

#include "ArrayView.h"
#include "Optional.h"
#include "StringView.h"
#include <set>
#include <string.h>
#include <string>

namespace Prime {

/// Allows a function that returns a const reference to return a reference to an empty string.
PRIME_PUBLIC extern const std::string emptyString;

PRIME_PUBLIC extern const StringView asciiWhitespaceChars;

PRIME_PUBLIC extern const StringView asciiNewlineChars;

PRIME_PUBLIC extern const StringView utf8WhitespaceChars;

PRIME_PUBLIC extern const StringView utf8NewlineChars;

PRIME_PUBLIC extern const StringView asciiAlphanumeric;

//
// ASCII queries
//

inline bool ASCIIIsUpper(int ch) { return ch >= 'A' && ch <= 'Z'; }
inline bool ASCIIIsLower(int ch) { return ch >= 'a' && ch <= 'z'; }
inline bool ASCIIIsAlpha(int ch) { return ASCIIIsUpper(ch) || ASCIIIsLower(ch); }
inline bool ASCIIIsDigit(int ch) { return ch >= '0' && ch <= '9'; }
inline bool ASCIIIsHexDigit(int ch) { return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'); }
inline bool ASCIIIsOctDigit(int ch) { return (ch >= '0' && ch <= '7'); }
inline bool ASCIIIsNewline(int ch) { return ch == '\r' || ch == '\n'; }
inline bool ASCIIIsSpaceOrTab(int ch) { return ch == ' ' || ch == '\t'; }
inline bool ASCIIIsSpaceOrTabOrNewline(int ch) { return ASCIIIsSpaceOrTab(ch) || ASCIIIsNewline(ch); }
inline bool ASCIIIsWhitespace(int ch) { return ch <= ' ' && ch > 0; }
inline bool ASCIIIsControlCharacter(int ch) { return ch < ' ' && ch >= 0; }
inline char ASCIIIsAlphanumeric(int ch) { return ASCIIIsAlpha(ch) || ASCIIIsDigit(ch); }
inline char ASCIIIsIdentifier(int ch) { return ASCIIIsAlpha(ch) || ASCIIIsDigit(ch) || ch == '_'; }

inline char ASCIIToUpper(int ch) { return ASCIIIsLower(ch) ? (char)(ch - ('a' - 'A')) : (char)ch; }
inline char ASCIIToLower(int ch) { return ASCIIIsUpper(ch) ? (char)(ch + ('a' - 'A')) : (char)ch; }

//
// Safe string copy/append from std::string/StringView
//

// Additional overloads of StringCopy and StringAppend are in Common.h

PRIME_PUBLIC bool StringCopy(char* buffer, size_t bufferSize, const std::string& string);
PRIME_PUBLIC bool StringCopy(char* buffer, size_t bufferSize, const std::string& string, size_t n);
PRIME_PUBLIC bool StringCopy(ArrayView<char> buffer, const std::string& string);
PRIME_PUBLIC bool StringCopy(ArrayView<char> buffer, const std::string& string, size_t n);
PRIME_PUBLIC bool StringAppend(char* buffer, size_t bufferSize, const std::string& string);
PRIME_PUBLIC bool StringAppend(char* buffer, size_t bufferSize, const std::string& string, size_t n);
PRIME_PUBLIC bool StringAppend(ArrayView<char> buffer, const std::string& string);
PRIME_PUBLIC bool StringAppend(ArrayView<char> buffer, const std::string& string, size_t n);

#if !defined(PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES)

template <size_t bufferSize>
bool StringCopy(char (&buffer)[bufferSize], const std::string& string)
{
    return StringCopy(buffer, bufferSize, string);
}

template <size_t bufferSize>
bool StringCopy(char (&buffer)[bufferSize], const std::string& string, size_t n)
{
    return StringCopy(buffer, bufferSize, string, n);
}

template <size_t bufferSize>
bool StringAppend(char (&buffer)[bufferSize], const std::string& string)
{
    return StringAppend(buffer, bufferSize, string);
}

template <size_t bufferSize>
bool StringAppend(char (&buffer)[bufferSize], const std::string& string, size_t n)
{
    return StringAppend(buffer, bufferSize, string, n);
}

#endif

template <typename Char, typename Traits>
bool StringCopy(Char* buffer, size_t bufferSize, const BasicStringView<Char, Traits>& piece)
{
    if (!PRIME_GUARD(bufferSize)) {
        return false;
    }

    size_t length = piece.size();
    bool fit;
    if (length > bufferSize - 1) {
        fit = false;
        length = bufferSize - 1;
    } else {
        fit = true;
    }

    memcpy(buffer, piece.data(), length);
    buffer[length] = 0;

    return fit;
}

template <typename Char, typename Traits>
inline bool StringCopy(ArrayView<Char> buffer, const BasicStringView<Char, Traits>& piece)
{
    return StringCopy(buffer.begin(), buffer.size(), piece);
}

template <typename Char, typename Traits>
bool StringAppend(Char* buffer, size_t bufferSize, const BasicStringView<Char, Traits>& piece)
{
    return StringAppend(buffer, bufferSize, piece.data(), piece.size());
}

template <typename Char, typename Traits>
inline bool StringAppend(ArrayView<Char> buffer, const BasicStringView<Char, Traits>& piece)
{
    return StringAppend(buffer.begin(), buffer.size(), piece.data(), piece.size());
}

#if !defined(PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES)

template <typename Char, typename Traits, size_t bufferSize>
bool StringCopy(Char (&buffer)[bufferSize], const BasicStringView<Char, Traits>& piece)
{
    return StringCopy(buffer, bufferSize, piece);
}

template <typename Char, typename Traits, size_t bufferSize>
bool StringAppend(Char (&buffer)[bufferSize], const BasicStringView<Char, Traits>& piece)
{
    return StringAppend(buffer, bufferSize, piece);
}

#endif

inline std::string& StringCopy(std::string& out, StringView in)
{
    out.assign(in.begin(), in.end());
    return out;
}

// These return true for compatibility with Convert.h

inline bool StringAppend(std::string& out, const char* in)
{
    out += in;
    return true;
}

inline bool StringAppend(std::string& out, const std::string& in)
{
    out += in;
    return true;
}

inline bool StringAppend(std::string& out, StringView in)
{
    out.append(in.begin(), in.end());
    return true;
}

//
// String utilities
//

template <typename Char>
size_t StringLength(const Char* string) { return std::char_traits<Char>::length(string); }

template <typename Char>
inline size_t SafeStringLength(const Char* string)
{
    return string ? StringLength<Char>(string) : 0;
}

/// Returns min(StringLength(string), maxLength) without accessing string[maxLength].
PRIME_PUBLIC size_t StringLength(const char* string, size_t maxLength);

//
// Safe string formatting
//

/// Returns a string object of type std::string containing the result of a sprintf.
PRIME_PUBLIC std::string Format(const char* format, ...);

/// vsprintf directly in to a std::string.
PRIME_PUBLIC void StringFormatVA(std::string& target, const char* format, va_list argptr);

/// sprintf directly in to a std::string.
PRIME_PUBLIC void StringFormat(std::string& target, const char* format, ...);

/// Appends sprintf formatted text on to a std::string.
PRIME_PUBLIC void StringAppendFormatVA(std::string& string, const char* format, va_list argptr);

/// Appends sprintf formatted text on to a std::string.
PRIME_PUBLIC void StringAppendFormat(std::string& string, const char* format, ...);

//
// FormatBuffer
//

namespace Private {

    PRIME_PUBLIC char* StringFormatAllocateOrUseBuffer(size_t& length, char* buffer, size_t bufferSize,
        const char* format, va_list argptr);

    static const size_t defaultFormatBufferSize = 128 - 2 * sizeof(void*);
}

/// Stores the result of a vsnprintf. Has an internal buffer which is used if large enough, otherwise memory will
/// be allocated. Usually, FormatBuffer is used instead of FormatBufferVA.
template <size_t bufferSize = Private::defaultFormatBufferSize>
class FormatBufferVA {
public:
    FormatBufferVA()
    {
        _buffer[0] = 0;
        _string = _buffer;
    }

    FormatBufferVA(const char* format, va_list argptr)
    {
        _string = Private::StringFormatAllocateOrUseBuffer(_length, _buffer, bufferSize, format, argptr);
    }

    ~FormatBufferVA()
    {
        free();
    }

    const char* formatVA(const char* format, va_list argptr)
    {
        free();
        _string = Private::StringFormatAllocateOrUseBuffer(_length, _buffer, bufferSize, format, argptr);
        return _string;
    }

    const char* format(const char* format, ...)
    {
        va_list argptr;
        va_start(argptr, format);
        formatVA(format, argptr);
        va_end(argptr);
        return _string;
    }

    /// Returns the null-terminated C string that contains the result.
    const char* c_str() const { return _string; }

    /// Returns the length of the formatted string.
    size_t getLength() const { return _length; }

    operator const char*() const { return _string; }

    operator std::string() const { return std::string(_string, getLength()); }

    operator StringView() const { return StringView(_string, getLength()); }

private:
    operator void*() const { return NULL; }

    void free()
    {
        if (_string != _buffer) {
            delete[] _string;
        }
    }

    char _buffer[bufferSize];
    char* _string;
    size_t _length;

    PRIME_UNCOPYABLE(FormatBufferVA);
};

/// Stores the result of an snprintf. Has an internal buffer which is used if large enough, otherwise memory will
/// be allocated. e.g., FormatBuffer<> message("File not found: %s", path); puts(message.c_str());
template <size_t bufferSize = Private::defaultFormatBufferSize>
class FormatBuffer : public FormatBufferVA<bufferSize> {
public:
    FormatBuffer() { }

    FormatBuffer(const char* format, ...)
    {
        va_list argptr;
        va_start(argptr, format);
        FormatBufferVA<bufferSize>::formatVA(format, argptr);
        va_end(argptr);
    }

    PRIME_UNCOPYABLE(FormatBuffer);
};

//
// NewString
//

#ifndef PRIME_COMPILER_NO_MEMBER_TEMPLATE_OVERRIDES

/// Returns a newly allocated copy of string, allocated using operator new[].
template <typename Char>
Char* NewString(const Char* string)
{
    if (!string) {
        return NULL;
    }

    size_t size = StringLength<Char>(string) + 1;

    Char* copy = new Char[size];

    if (copy) {
        memcpy(copy, string, size * sizeof(Char));
    }

    return copy;
}

/// Returns a newly allocated copy of a number of characters, allocated using operator new[].
template <typename Char>
Char* NewString(const Char* string, size_t length)
{
    if (!string) {
        return NULL;
    }

    Char* copy = new Char[length + 1];

    if (copy) {
        memcpy(copy, string, length * sizeof(Char));
        copy[length] = 0;
    }

    return copy;
}

#endif

/// Returns a newly allocated copy of string, allocated using operator new[].
PRIME_PUBLIC char* NewString(const char* string);

/// Returns a newly allocated copy of a number of characters, allocated using operator new[].
PRIME_PUBLIC char* NewString(const char* string, size_t length);

//
// String comparisons
//

PRIME_PUBLIC int ASCIICompareIgnoringCase(const char* a, const char* b);

PRIME_PUBLIC int ASCIICompareIgnoringCase(const char* a, const char* b, size_t n);

PRIME_PUBLIC int ASCIICompareIgnoringCase(const char* a, size_t aSize, const char* b, size_t bSize);

inline int ASCIICompareIgnoringCase(StringView lhs, StringView rhs)
{
    return ASCIICompareIgnoringCase(lhs.data(), lhs.size(), rhs.data(), rhs.size());
}

PRIME_PUBLIC bool ASCIIEqualIgnoringCase(const char* a, size_t aSize, const char* b, size_t bSize);
PRIME_PUBLIC bool ASCIIEqualIgnoringCase(const char* begin, const char* end, const char* other);

inline bool ASCIIEqualIgnoringCase(StringView lhs, StringView rhs)
{
    return lhs.size() == rhs.size() && ASCIIEqualIgnoringCase(lhs.begin(), lhs.end(), rhs.begin());
}

/// Strings must be identical, so for UTF-8 it's often best to use CaseStartsWith
inline bool StringStartsWith(StringView string, StringView with)
{
    return string.size() >= with.size() && std::equal(with.begin(), with.end(), string.begin());
}

inline bool StringStartsWith(StringView string, char ch)
{
    return !string.empty() && string.front() == ch;
}

/// Strings must be identical, so for UTF-8 it's often best to use CaseEndsWith
inline bool StringEndsWith(StringView string, StringView with)
{
    return string.size() >= with.size() && std::equal(with.begin(), with.end(), string.end() - with.size());
}

inline bool StringEndsWith(StringView string, char ch)
{
    return !string.empty() && string.back() == ch;
}

inline bool ASCIIStartsWithIgnoringCase(StringView string, StringView with)
{
    return string.size() >= with.size() && ASCIIEqualIgnoringCase(with.begin(), with.end(), string.begin());
}

inline bool ASCIIEndsWithIgnoringCase(StringView string, StringView with)
{
    return string.size() >= with.size() && ASCIIEqualIgnoringCase(with.begin(), with.end(), string.end() - with.size());
}

inline bool StringsEqual(StringView lhs, StringView rhs)
{
    return lhs == rhs;
}

inline bool StringIsEmpty(const char* string)
{
    return !string || !*string;
}

inline bool StringIsEmpty(const std::string& string)
{
    return string.empty();
}

inline bool StringIsEmpty(StringView string)
{
    return string.empty();
}

/// Returns true if the string contains non-ASCII characters (> 0x7f).
PRIME_PUBLIC bool ContainsExtendedCharacters(StringView string);

/// Some algorithms have optimised paths for non-UTF8 strings.
enum UTF8Mode {
    UTF8ModeUnknown = -1,
    UTF8ModeASCII = 0,
    UTF8ModeUTF8 = 1
};

//
// Wildcards
//

/// Simple wildcard matching. '*' matches any number of characters and '?' matches a single character.
PRIME_PUBLIC bool WildcardMatch(StringView wildcard, StringView string, bool ignoreCase);

/// Simple wildcard matching for paths. '*' matches any number of characters up until the next separator, '?'
/// matches a single character except a separator, and '**' matches any number of characters, skipping separators.
PRIME_PUBLIC bool WildcardMatch(StringView wildcard, StringView string, bool ignoreCase,
    StringView separators); // TODO: UTF-8 support for separators

//
// Parsing
//

// UTF-8 versions of the below are missing, although many have better alternatives (UTF8FindFirstOf, etc.)

PRIME_PUBLIC const char* ASCIISkipSpacesAndTabs(const char* string, const char* end) PRIME_NOEXCEPT;
PRIME_PUBLIC const char* ASCIISkipSpacesAndTabs(const char* string) PRIME_NOEXCEPT;

/// Reverse ptr until the character before it is not a space or tab.
PRIME_PUBLIC const char* ASCIIReverseSkipSpacesAndTabs(const char* begin, const char* ptr) PRIME_NOEXCEPT;

/// Skips all whitespace, including newlines.
PRIME_PUBLIC const char* ASCIISkipWhitespace(const char* string, const char* end) PRIME_NOEXCEPT;
PRIME_PUBLIC const char* ASCIISkipWhitespace(const char* string) PRIME_NOEXCEPT;

/// Skip space, tab, CR and LF.
PRIME_PUBLIC const char* ASCIISkipSpacesTabsAndNewlines(const char* string, const char* end) PRIME_NOEXCEPT;
PRIME_PUBLIC const char* ASCIISkipSpacesTabsAndNewlines(const char* string) PRIME_NOEXCEPT;

/// Returns a pointer to the next newline in [string, end).
PRIME_PUBLIC const char* ASCIISkipUntilNewline(const char* string, const char* end) PRIME_NOEXCEPT;
PRIME_PUBLIC const char* ASCIISkipUntilNewline(const char* string) PRIME_NOEXCEPT;

/// With *string pointing to a newline sequence comprised of \r and \n characters, returns a pointer past the
/// newline sequence. If the input string is empty, returns the string unchanged.
PRIME_PUBLIC const char* ASCIISkipNewline(const char* string, const char* end) PRIME_NOEXCEPT;
PRIME_PUBLIC const char* ASCIISkipNewline(const char* string) PRIME_NOEXCEPT;

/// Skips all characters until it encounters a newline, then skips the newline. Deals with \r\n, \n, \n\r and \r.
PRIME_PUBLIC const char* ASCIISkipNextNewline(const char* string, const char* end) PRIME_NOEXCEPT;
PRIME_PUBLIC const char* ASCIISkipNextNewline(const char* string) PRIME_NOEXCEPT;

/// Returns a pointer to the next space or tab in [string, end).
PRIME_PUBLIC const char* ASCIISkipUntilSpaceOrTab(const char* ptr, const char* end) PRIME_NOEXCEPT;
PRIME_PUBLIC const char* ASCIISkipUntilSpaceOrTab(const char* ptr) PRIME_NOEXCEPT;

/// Skips whitespace, then if the next character is separator, skip that followed by any more whitespace. If
/// separator is zero, it is never matched. So for " , 8", ",8", and "8", this function will return a pointer to
/// the '8'.
PRIME_PUBLIC const char* ASCIISkipWhitespaceForArray(const char* string, const char* end, char separator = ',') PRIME_NOEXCEPT;
PRIME_PUBLIC const char* ASCIISkipWhitespaceForArray(const char* string, char separator = ',') PRIME_NOEXCEPT;

/// Returns a pointer to the first character in string which isn't ch.
/// Use UTF8FindFirstNotOf for UTF-8.
PRIME_PUBLIC const char* ASCIISkipChar(const char* string, const char* end, char ch) PRIME_NOEXCEPT;
PRIME_PUBLIC const char* ASCIISkipChar(const char* string, char ch) PRIME_NOEXCEPT;

/// Returns a pointer to the first character in string which isn't in chars.
/// Use UTF8FindFirstNotOf for UTF-8.
PRIME_PUBLIC const char* ASCIISkipChars(const char* string, const char* end, StringView chars) PRIME_NOEXCEPT;
PRIME_PUBLIC const char* ASCIISkipChars(const char* string, StringView chars) PRIME_NOEXCEPT;

/// Returns a pointer to the start of the string or to the first character after the last of any of the separators.
/// e.g., StringLastComponent("/path/to/file", "/") will return "file".
PRIME_PUBLIC const char* StringLastComponent(const char* string, const char* end, StringView separators, UTF8Mode utf8Mode = UTF8ModeUnknown) PRIME_NOEXCEPT;
PRIME_PUBLIC const char* StringLastComponent(const char* string, StringView separators, UTF8Mode utf8Mode = UTF8ModeUnknown) PRIME_NOEXCEPT;

//
// TokenParser
//

/// Reads series of tokens separated by single character separators. Aware of "double quotes" and optionally
/// aware of \backslash escaped characters. There can be whitespace on either side of a separator.
class PRIME_PUBLIC TokenParser {
public:
    enum {
        /// If set, a backslash (\\) will escape the next character. The backslash is not stripped from the output
        /// or converted in any way (you can use CUnescape on the resulting token).
        OptionBackslashIsEscape = 1u
    };

    explicit TokenParser(StringView string);

    TokenParser();

    /// Set the string to be parsed.
    void init(StringView string);

    /// Returns false at the end of the string.
    bool parse(StringView& token, const char* separators, unsigned int options = 0);

    /// Returns false if the parsed string didn't fit in tokenBuffer or we reached the end of the string. Use
    /// atEnd() to disambiguate.
    bool parse(ArrayView<char> tokenBuffer, const char* separators, unsigned int options = 0);

    bool atEnd() const { return !_ok; }

    /// The token that was last parsed.
    StringView getToken() const { return _token; }

    /// The separator that ended the last token, or 0 for the end of the string.
    char getSeparator() const { return _separator; }

    StringView getRemainingString() const { return _string; }

    void putBack() { _string = _stringWas; }

private:
    void construct();

    StringView _string;
    StringView _stringWas;
    StringView _token;
    char _separator;
    bool _ok;
};

//
// WordParser
//

/// Parses contiguous words from a string. Words are defined by IsWordChar.
template <typename IsWordChar>
class BasicWordParser {
public:
    explicit BasicWordParser(StringView string, IsWordChar isWordChar = IsWordChar())
        : _ptr(string.begin())
        , _end(string.end())
        , _isWordChar(isWordChar)
    {
    }

    Optional<StringView> next()
    {
        const char* next;
        while (_ptr != _end && !_isWordChar(_ptr, _end, next)) {
            _ptr = next;
        }

        if (_ptr == _end) {
            return nullopt;
        }

        const char* start = _ptr;
        do {
            _ptr = next;
        } while (_ptr != _end && _isWordChar(_ptr, _end, next));

        return StringView(start, _ptr);
    }

private:
    const char* _ptr;
    const char* _end;
    IsWordChar _isWordChar;
};

class ASCIIIsWordChar {
public:
    inline bool operator()(const char* ptr, const char* end, const char*& next)
    {
        (void)end;
        next = ptr + 1;
        return ASCIIIsAlphanumeric(*ptr);
    }
};

/// Recognises UTF-8 whitspace and only considers ASCII non-alphanumeric characters to be non-alphanumeric.
class PRIME_PUBLIC HybridIsWordChar {
public:
    bool operator()(const char* ptr, const char* end, const char*& next);
};

typedef BasicWordParser<ASCIIIsWordChar> ASCIIWordParser;
typedef BasicWordParser<HybridIsWordChar> HybridWordParser;

//
// String trimming
//

PRIME_PUBLIC void StringRightTrimInPlace(std::string& string, StringView whitespace = utf8WhitespaceChars, UTF8Mode utf8Mode = UTF8ModeUnknown);

PRIME_PUBLIC void StringLeftTrimInPlace(std::string& string, StringView whitespace = utf8WhitespaceChars, UTF8Mode utf8Mode = UTF8ModeUnknown);

PRIME_PUBLIC void StringTrimInPlace(std::string& string, StringView whitespace = utf8WhitespaceChars, UTF8Mode utf8Mode = UTF8ModeUnknown);

PRIME_PUBLIC StringView StringViewRightTrim(StringView string, StringView whitespace = utf8WhitespaceChars, UTF8Mode utf8Mode = UTF8ModeUnknown);

PRIME_PUBLIC StringView StringViewLeftTrim(StringView string, StringView whitespace = utf8WhitespaceChars, UTF8Mode utf8Mode = UTF8ModeUnknown);

PRIME_PUBLIC StringView StringViewTrim(StringView string, StringView whitespace = utf8WhitespaceChars, UTF8Mode utf8Mode = UTF8ModeUnknown);

inline std::string StringRightTrim(StringView string, StringView whitespace = utf8WhitespaceChars, UTF8Mode utf8Mode = UTF8ModeUnknown)
{
    return StringViewRightTrim(string, whitespace, utf8Mode).to_string();
}

inline std::string StringLeftTrim(StringView string, StringView whitespace = utf8WhitespaceChars, UTF8Mode utf8Mode = UTF8ModeUnknown)
{
    return StringViewLeftTrim(string, whitespace, utf8Mode).to_string();
}

inline std::string StringTrim(StringView string, StringView whitespace = utf8WhitespaceChars, UTF8Mode utf8Mode = UTF8ModeUnknown)
{
    return StringViewTrim(string, whitespace, utf8Mode).to_string();
}

/// Returns true if the string contains only whitespace, or is empty.
PRIME_PUBLIC bool StringIsWhitespace(StringView string, StringView whitespace = utf8WhitespaceChars, UTF8Mode utf8Mode = UTF8ModeUnknown);

PRIME_PUBLIC void MiddleTruncateStringInPlace(std::string& string, size_t maxSize, const std::string& ellipsis);

//
// String replace
//

PRIME_PUBLIC void StringReplaceInPlace(std::string& string, StringView replaceThis, StringView withThis,
    size_t startSearch = 0);

PRIME_PUBLIC std::string StringReplace(StringView string, StringView replaceThis, StringView withThis,
    size_t startSearch = 0);

PRIME_PUBLIC bool StringReplaceInPlace(char* buffer, size_t bufferSize, StringView source,
    StringView replaceThis, StringView withThis);

/// Wrapper around string.find()
PRIME_PUBLIC StringView::size_type StringFind(StringView string, StringView findThis, size_t startSearch = 0);

PRIME_PUBLIC StringView::size_type ASCIIFindIgnoringCase(StringView string, StringView findThis, size_t startSearch = 0);

// Missing: ASCIIReplaceIgnoringCase, ASCIIReplaceIgnoringCase, StringCaseReplace, CaseReplace

/// Reads the first substring of the form [+-]\d+(.\d+)?
PRIME_PUBLIC std::string StringExtractNumber(StringView string);

/// Returns a new string containing only ASCII numeric characters.
PRIME_PUBLIC std::string ASCIIOnlyNumeric(StringView string);

/// Returns a new string containing only ASCII alphanumeric characters.
PRIME_PUBLIC std::string ASCIIOnlyAlphanumeric(StringView string);

/// Returns a new string containing only ASCII alphanumeric characters.
PRIME_PUBLIC std::string ASCIIOnlyAlphanumericUppercase(StringView string);

//
// CaseConverter
//

/// Abstract away case conversion code, so we can plug in ICU or utf8rewind, for example. The methods of this
/// class aren't meant to be used directly - use the functions such as StringLower or StringToTitleCase.
class PRIME_PUBLIC CaseConverter {
public:
    /// The global CaseConverter. By default, only ASCII is supported.
    static CaseConverter* getGlobal() { return _global ? _global : getASCIIConverter(); }

    static void setGlobal(CaseConverter* log) { _global = log; }

    static CaseConverter* getASCIIConverter();

    virtual ~CaseConverter();

    /// Returns the required number of bytes to convert the entire source string. If dest is NULL, no output
    /// is written but the return value is still correct.
    virtual size_t toUpperCase(StringView source, char* dest, size_t destSize) = 0;

    /// Returns the required number of bytes to convert the entire source string. If dest is NULL, no output
    /// is written but the return value is still correct.
    virtual size_t toLowerCase(StringView source, char* dest, size_t destSize) = 0;

    /// Returns the required number of bytes to convert the entire source string. If dest is NULL, no output
    /// is written but the return value is still correct.
    virtual size_t toTitleCase(StringView source, char* dest, size_t destSize) = 0;

    /// Returns the required number of bytes to convert the entire source string. If dest is NULL, no output
    /// is written but the return value is still correct.
    virtual size_t fold(StringView source, char* dest, size_t destSize) = 0;

private:
    static CaseConverter* _global;
};

//
// Case conversions
//

/// Converts "hello world" to "Hello World".
PRIME_PUBLIC void ASCIIToTitleCaseInPlace(const char* begin, const char* end, char* out);

PRIME_PUBLIC void ASCIIToLowerInPlace(char* string);
PRIME_PUBLIC void ASCIIToUpperInPlace(char* string);
PRIME_PUBLIC void ASCIIToTitleCaseInPlace(char* string);

PRIME_PUBLIC void ASCIIToLowerInPlace(std::string& string);
PRIME_PUBLIC void ASCIIToUpperInPlace(std::string& string);
PRIME_PUBLIC void ASCIIToTitleCaseInPlace(std::string& string);

PRIME_PUBLIC std::string ASCIIToLower(StringView string);
PRIME_PUBLIC std::string ASCIIToUpper(StringView string);
PRIME_PUBLIC std::string ASCIIToTitleCase(StringView string);

PRIME_PUBLIC void StringToLowerInPlace(std::string& string);
PRIME_PUBLIC void StringToUpperInPlace(std::string& string);
PRIME_PUBLIC void StringToTitleCaseInPlace(std::string& string);
PRIME_PUBLIC void StringCaseFoldInPlace(std::string& string);

PRIME_PUBLIC std::string StringToLower(StringView string);
PRIME_PUBLIC std::string StringToUpper(StringView string);
PRIME_PUBLIC std::string StringToTitleCase(StringView string);
PRIME_PUBLIC std::string StringCaseFold(StringView string);

//
// UNICODE caseless comparisons
//

PRIME_PUBLIC bool StringsEqualIgnoringCase(StringView a, StringView b);
PRIME_PUBLIC int StringCompareIngoringCase(StringView a, StringView b);

// Missing: CaseStartsWith, CaseEndsWith, using case folding

//
// String splitting (joining is in Convert.h)
//

typedef std::pair<StringView, StringView> StringViewPair;

PRIME_PUBLIC StringViewPair StringViewBisectLine(StringView view) PRIME_NOEXCEPT;

PRIME_PUBLIC StringViewPair StringViewBisect(StringView string, char ch);

PRIME_PUBLIC StringViewPair StringViewBisect(StringView string, StringView separator);

PRIME_PUBLIC StringViewPair StringViewBisectOnSeparators(StringView string, StringView separators, UTF8Mode utf8Mode = UTF8ModeUnknown);

PRIME_PUBLIC StringViewPair StringViewReverseBisect(StringView string, char ch);

PRIME_PUBLIC StringViewPair StringViewReverseBisect(StringView string, StringView separator);

PRIME_PUBLIC StringViewPair StringViewReverseBisectOnSeparators(StringView string, StringView separators, UTF8Mode utf8Mode = UTF8ModeUnknown);

enum {
    SplitKeepWhitespace = 1u << 0,
    SplitSkipEmpty = 1u << 1,
};

PRIME_PUBLIC void StringSplit(std::vector<std::string>& vector, StringView string, StringView separator,
    unsigned int flags = 0);

PRIME_PUBLIC std::vector<std::string> StringSplit(StringView string, StringView separator, unsigned int flags = 0);

PRIME_PUBLIC void StringSplitOnSeparators(std::vector<std::string>& vector, StringView string, StringView separators,
    unsigned int flags = 0, UTF8Mode utf8Mode = UTF8ModeUnknown);

PRIME_PUBLIC std::vector<std::string> StringSplitOnSeparators(StringView string, StringView separators,
    unsigned int flags = 0, UTF8Mode utf8Mode = UTF8ModeUnknown);

PRIME_PUBLIC void StringSplit(std::vector<StringView>& vector, StringView string, StringView separator,
    unsigned int flags = 0);

PRIME_PUBLIC std::vector<StringView> StringViewSplit(StringView string, StringView separator, unsigned int flags = 0);

PRIME_PUBLIC void StringSplitOnSeparators(std::vector<StringView>& vector, StringView string, StringView separators,
    unsigned int flags = 0, UTF8Mode utf8Mode = UTF8ModeUnknown);

PRIME_PUBLIC std::vector<StringView> SplitSeparatorsViews(StringView string, StringView separators,
    unsigned int flags = 0, UTF8Mode utf8Mode = UTF8ModeUnknown);

PRIME_PUBLIC std::string StringRepeat(StringView string, size_t count);

PRIME_PUBLIC std::vector<StringView> StringViewSplitLines(StringView string);

PRIME_PUBLIC std::vector<std::string> StringSplitLines(StringView string);

//
// String substitution
//

class PRIME_PUBLIC VariableExpander {
public:
    virtual ~VariableExpander();

    virtual void appendVariable(std::string& target, const char* variableName, char brace) const = 0;
};

/// Performs shell style expansion of variable names, e.g., $(name) or $name, within a string. $symbol is also
/// supported, e.g., $/, so you may define a variable with the name "/" to expand to the path character, for
/// example. $$ is special cased to allow you to escape a $. Target is any type which has an `append(begin, end)`
/// method (e.g., std::string).
PRIME_PUBLIC void StringExpandDollarVariablesInPlace(std::string& target, StringView string, const VariableExpander& expander);

PRIME_PUBLIC std::string StringExpandDollarVariables(StringView string, const VariableExpander& expander);

//
// Fuzzy comparisons
//

/// Replaces all non-alphanumberic characters with spaces, splits the string on spaces (removing empty
/// substrings) then removes any stop words ("and", etc.).
PRIME_PUBLIC std::vector<std::string> ASCIIToWords(StringView string,
    const std::set<std::string>& stopwords = std::set<std::string>());

/// Like ASCIIToWords but is aware of UTF-8 whitespace and ASCII word characters.
PRIME_PUBLIC std::vector<std::string> HybridToWords(StringView string,
    const std::set<std::string>& stopwords = std::set<std::string>());

/// Compares two strings for equality ignoring anything non-alphanumeric. So 0123 456 789 will match 0123-456789.
PRIME_PUBLIC bool StringsEqualIgnoringNonAlphanumeric(StringView a, StringView b);

/// Returns true if the string contains no alphanumeric characters. UTF-8 aware, but only searches for ASCII alphanumeric.
PRIME_PUBLIC bool ASCIIContainsNonAlphanumeric(StringView string, UTF8Mode utf8Mode = UTF8ModeUnknown);

/// Converts "Elliot John Lord" to "EJL"
PRIME_PUBLIC std::string StringToInitials(StringView string);
}

#endif
