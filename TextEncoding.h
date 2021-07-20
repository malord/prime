// Copyright 2000-2021 Mark H. P. Lord

// Encoding and decoding text/bytes. No dependencies on external libraries.

#ifndef PRIME_TEXTENCODING_H
#define PRIME_TEXTENCODING_H

#include "ArrayView.h"
#include "Config.h"
#include "StringView.h"
#include <set>

namespace Prime {

//
// UTF-8
//

class PRIME_PUBLIC UTF8Decoder {
public:
    UTF8Decoder(const void* src, size_t srcLength, bool strict)
        : _src(reinterpret_cast<const uint8_t*>(src))
        , _srcLength(srcLength)
        , _strict(strict)
        , _valid(true)
    {
    }

    UTF8Decoder(StringView src, bool strict)
        : _src(reinterpret_cast<const uint8_t*>(src.begin()))
        , _srcLength(src.size())
        , _strict(strict)
        , _valid(true)
    {
    }

    /// Returns true if a character was read, false if there are no more characters to read. On error, ignores a
    /// single byte then returns true. To check for errors, use wasValid().
    bool read(uint32_t& ch);

    bool hasFinished() const { return _srcLength == 0; }

    /// A decode was invalid if strict was requested in the constructor and a non-shortest-form encoding
    /// was found, or if there was an incomplete character at the end of the input.
    bool wasValid() const { return _valid; }

private:
    const uint8_t* _src;
    size_t _srcLength;
    bool _strict;
    bool _valid;
};

/// Returns the number of bytes required to encode a Unicode character as UTF-8.
PRIME_PUBLIC unsigned int UTF8EncodedLength(uint32_t ch);

/// Encode a Unicode character as UTF-8, writing up to 6 bytes to *dest. Returns the number of bytes written.
PRIME_PUBLIC size_t UTF8Encode(void* dest, uint32_t ch);

/// Decode the leading byte of a possibly UTF-8 encoded sequence of bytes. Returns the total number of bytes the
/// UTF-8 sequence requires, including this byte. Returns 0 on error.
PRIME_PUBLIC unsigned int UTF8DecodeLengthFromLeadingByte(uint8_t ch);

/// Verify that all the trailing bytes of a multi-byte UTF-8 character are actually trailing bytes. Does not check
/// the first byte.
PRIME_PUBLIC bool UTF8VerifyTrailingBytes(const void* bytes, unsigned int len);

/// Decode a UTF-8 multi-byte character of the specified encoded length in bytes.
PRIME_PUBLIC uint32_t UTF8Decode(const void* bytes, unsigned int len);

/// Determine whether there is a valid UTF-8 character at *ptr. If so, returns true and sets *len.
PRIME_PUBLIC bool UTF8IsValid(const void* ptr, const void* end, unsigned int* len);

PRIME_PUBLIC const char* UTF8ReverseFindLeadingByte(const void* ptr, const void* begin);

PRIME_PUBLIC StringView::size_type UTF8FindCodePoint(StringView string, uint32_t codepoint);

PRIME_PUBLIC StringView::size_type UTF8FindLastOf(StringView string, StringView chars);

PRIME_PUBLIC StringView::size_type UTF8FindFirstOf(StringView string, StringView chars);

PRIME_PUBLIC StringView::size_type UTF8FindLastNotOf(StringView string, StringView chars);

PRIME_PUBLIC StringView::size_type UTF8FindFirstNotOf(StringView string, StringView chars);

/// Skip numberOfCodepoints from the start of the supplied string.
PRIME_PUBLIC const char* UTF8Advance(const void* ptr, const void* end, size_t numberOfCodepoints);

/// Return a new StringView with the first numberOfCodepoints stripped.
PRIME_PUBLIC StringView UTF8Advance(StringView string, size_t numberOfCodepoints);

//
// UTF-16
//

class PRIME_PUBLIC UTF16Decoder {
public:
    UTF16Decoder(const uint16_t* src, size_t srcLength)
        : _src(src)
        , _srcEnd(src + srcLength)
        , _valid(true)
    {
    }

    /// Returns true if a character was read, false if there are no more characters to read. On error, ignores a
    /// single uint16 then returns true. To check for errors, use wasValid().
    bool read(uint32_t& ch);

    bool hasFinished() const { return _src == _srcEnd; }

    /// A decode was invalid if there was a missing second part of a two character sequence or an invalid
    /// trailing uint16 was found.
    bool wasValid() const { return _valid; }

private:
    const uint16_t* _src;
    const uint16_t* _srcEnd;
    bool _valid;
};

inline bool UTF16IsLeadingWord(uint32_t ch)
{
    return ch >= 0xD800u && ch <= 0xDBFFu;
}

inline bool UTF16IsTrailingWord(uint32_t ch)
{
    return ch >= 0xDC00u && ch <= 0xDFFFu;
}

inline bool UTF16CanEncode(uint32_t ch)
{
    return ch <= 0x10fffd && !(ch >= 0xd800 && ch <= 0xdfff);
}

/// Returns the number of uint16s required to encode a Unicode character as UTF-16. The character must be
/// encodable (use the PRIME_UTF16_CAN_ENCODE macro).
PRIME_PUBLIC unsigned int UTF16EncodedLength(uint32_t ch);

/// Encode a Unicode character as UTF-16. The character must be encodable as UTF-16 (use the
/// PRIME_UTF16_CAN_ENCODE macro). Returns the number of uint16s that were written.
PRIME_PUBLIC unsigned int UTF16Encode(uint16_t* dest, uint32_t ch);

PRIME_PUBLIC void UTF16ByteSwap(uint16_t* buffer, size_t len);

//
// Unicode conversion
//

/// If dest is null then the length of the output is computed, but nothing is actually written. The caller must
/// ensure there's room for the output. Returns the number of uint16s that were (or would be) written.
PRIME_PUBLIC size_t UTF8ToUTF16(const void* src, size_t srcLength, uint16_t* dest, bool* isValid = NULL);

/// If dest is null then the length of the output is computed, but nothing is actually written. The caller must
/// ensure there's room for the output. Returns the number of uint32_ts that were (or would be) written.
PRIME_PUBLIC size_t UTF8ToUCS4(const void* src, size_t srcLength, uint32_t* dest, bool* isValid = NULL);

/// If dest is null then the length of the output is computed, but nothing is actually written. The caller must
/// ensure there's room for the output. Returns the number of bytes that were (or would be) written.
PRIME_PUBLIC size_t UTF16ToUTF8(const uint16_t* src, size_t srcLength, void* dest, bool* isValid = NULL);

/// If dest is null then the length of the output is computed, but nothing is actually written. The caller must
/// ensure there's room for the output. Returns the number of uint32_ts that were (or would be) written.
PRIME_PUBLIC size_t UTF16ToUCS4(const uint16_t* src, size_t srcLength, uint32_t* dest, bool* isValid = NULL);

/// If dest is null then the length of the output is computed, but nothing is actually written. The caller must
/// ensure there's room for the output. Returns the number of bytes that were (or would be) written.
PRIME_PUBLIC size_t UCS4ToUTF8(const uint32_t* src, size_t srcLength, void* dest, bool* isValid = NULL);

/// If dest is null then the length of the output is computed, but nothing is actually written. The caller must
/// ensure there's room for the output. Returns the number of uint16s that were (or would be) written.
PRIME_PUBLIC size_t UCS4ToUTF16(const uint32_t* src, size_t srcLength, uint16_t* dest, bool* isValid = NULL);

//
// Unicode information
//

namespace Private {
    PRIME_PUBLIC bool IsUnicodeWhitespace2(uint32_t codepoint);
}

inline bool IsUnicodeWhitespace(uint32_t codepoint)
{
    if ((codepoint & UINT32_C(0xffffff00)) == 0) {
        return codepoint == 0x09 || codepoint == 0x0a || codepoint == 0x0d || codepoint == 0x0b || codepoint == 0x0c || codepoint == 0x20 || codepoint == 0x85 || codepoint == 0xa0;
    }

    return Private::IsUnicodeWhitespace2(codepoint);
}

inline bool IsUnicodeNewline(uint32_t codepoint)
{
    return codepoint == 0x000a || codepoint == 0x000b || codepoint == 0x000c || codepoint == 0x000d || codepoint == 0x0085 || codepoint == 0x2028 || codepoint == 0x2029;
}

//
// CEscape/CUnescape
//

enum CEscapeFlags {
    /// Don't use \x for non-ASCII characters.
    CEscapeFlagNoHex = 1u << 0,

    /// Don't use \0 for \0, strip them instead.
    CEscapeFlagStripNulls = 1u << 1,

    /// Content is UTF-8 and characters beyond 0x80 should be escaped (ignored if CEscapeFlagNoHex is used).
    CEscapeFlagEscapeUTF8 = 1u << 2,

    /// Content is UTF-8 and should be passed through unescaped.
    CEscapeFlagKeepUTF8 = 1u << 3,

    CEscapeFlagDoubleQuotes = 1u << 4,
    CEscapeFlagApostrophes = 1u << 5,
    CEscapeFlagSpaces = 1u << 6,
    CEscapeFlagAsterisks = 1u << 7,
    CEscapeFlagQuestionMarks = 1u << 8,
    CEscapeFlagA = 1u << 9, // \a
    CEscapeFlagB = 1u << 10, // \b
    CEscapeFlagF = 1u << 11, // \f
    CEscapeFlagN = 1u << 12, // \n
    CEscapeFlagR = 1u << 13, // \r
    CEscapeFlagT = 1u << 14, // \t
    CEscapeFlagV = 1u << 15, // \v

    /// Use Unicode (\\u) escapes, rather than \\x escapes. This is implied by CEscapeFlagEscapeUTF8 and CEscapeFlagKeepUTF8.
    CEscapeFlagUnicode = 1u << 20,

    CEscapeFlagsAllCodes = CEscapeFlagA | CEscapeFlagB | CEscapeFlagF | CEscapeFlagN | CEscapeFlagR | CEscapeFlagT | CEscapeFlagV,

    CEscapeFlagsAllShellNotWildcards = CEscapeFlagDoubleQuotes | CEscapeFlagApostrophes | CEscapeFlagSpaces,

    CEscapeFlagsAllShell = CEscapeFlagsAllShellNotWildcards | CEscapeFlagAsterisks | CEscapeFlagQuestionMarks,

};

/// Reads srcLength characters from src and backslash-escapes any characters deemed to need escaping (the
/// backslash character itself, and everything specified by the flags). Writes to buffer, if it's not null.
/// Returns the number of characters written (or which would be written, if buffer is null). If return value
/// >= bufferSize hen the output was truncated.
PRIME_PUBLIC size_t CEscape(char* buffer, size_t bufferSize, StringView src, unsigned int flags);

/// Decodes C escape sequence in a string (e.g., \n, \x7f, \0) and writes the result to the supplied buffer,
/// then returns the total decoded length (which may exceed the size of the buffer, although the buffer will
/// never be exceeded and will always be null terminated). If no buffer is supplied, the unescaped length is
/// computed and returned without writing the result anywhere. Note that Unicode escapes (e.g., \u2014) are
/// converted to UTF-8.
PRIME_PUBLIC size_t CUnescape(char* buffer, size_t bufferSize, StringView src);

//
// Base-64
//

/// Compute the maximum size in bytes that an array of bytes of length inputSize would be encoded as in
/// Base-64 encoding. If lineLength is not zero, then the number of lines that would be output is also computed,
/// and the size of the newlines is added to the result.
PRIME_PUBLIC size_t Base64ComputeMaxEncodedSize(size_t inputSize, int lineLength = 0, size_t newlineSize = 0);

/// Base-64 encode sourceSize bytes from the memory pointed to by source in to the buffer specified by buffer.
/// For every lineLength bytes of output, a newline will be written to the output (lineLength can be zero to
/// disable newlines). Returns the number of bytes written to the buffer. If the buffer is a NULL pointer, nothing
/// is written, but the function returns the number of bytes that would be written.
/// DOES NOT NULL TERMINATE THE OUTPUT.
PRIME_PUBLIC size_t Base64Encode(void* buffer, size_t bufferSize, const void* source, size_t sourceSize,
    int lineLength = 0, StringView newline = "");

/// Computes the maximum number of bytes that Base-64 input of the specified size in bytes could be decoded to.
PRIME_PUBLIC size_t Base64ComputeMaxDecodedSize(size_t inputSize);

/// Decode Base-64 encoded data of sourceSize bytes in length from the memory pointed to by source in to the
/// memory pointed to by buffer. Returns the number of bytes decoded, or -1 if the buffer is too small.
PRIME_PUBLIC ptrdiff_t Base64Decode(void* buffer, size_t bufferSize, StringView source);

/// Manually build the Base-64 decoding table. This is normally handled for you by Base64Decode(), but in
/// a multithreaded application calling this can prevent a race condition.
PRIME_PUBLIC void Base64BuildDecodingTable();

/// Wrapper around Base64Encode that produces a std::string.
PRIME_PUBLIC std::string Base64Encode(const void* source, size_t sourceSize, int lineLength = 0,
    StringView newline = "");

PRIME_PUBLIC void Base64EncodeAppend(std::string& out, const void* data, size_t size);

PRIME_PUBLIC void Base64EncodeAppend(std::string& out, StringView string);

PRIME_PUBLIC bool Base64DecodeAppend(std::string& out, StringView string);

PRIME_PUBLIC std::string Base64Encode(StringView source);

PRIME_PUBLIC std::string Base64Decode(StringView source);

namespace Base64 {

    PRIME_PUBLIC extern const uint8_t encodingTable[65];

    const uint8_t padChar = '=';

    PRIME_PUBLIC extern uint8_t decodingTable[256];

    PRIME_PUBLIC void BuildDecodingTable();

    const uint8_t decodingTableInvalidChar = 0xff;
    const uint8_t decodingTablePadChar = 0xfe;

    inline void EncodeBlock(uint8_t out[4], const uint8_t in[3])
    {
        out[0] = encodingTable[in[0] >> 2];
        out[1] = encodingTable[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
        out[2] = encodingTable[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)];
        out[3] = encodingTable[in[2] & 0x3f];
    }

    inline void EncodeBlock(uint8_t out[4], const uint8_t in[], unsigned int inLength)
    {
        switch (inLength) {
        case 3:
            EncodeBlock(out, in);
            break;

        case 2:
            PRIME_DEBUG_ASSERT(in[2] == 0);
            EncodeBlock(out, in);
            out[3] = padChar;
            break;

        case 1:
            PRIME_DEBUG_ASSERT(in[1] == 0 && in[2] == 0);
            EncodeBlock(out, in);
            out[2] = padChar;
            out[3] = padChar;
            break;

        default:
            PRIME_DEBUG_ASSERT(0);
            break;
        }
    }

    inline void DecodeBlock(uint8_t* out, const uint8_t* in)
    {
        out[0] = (uint8_t)(in[0] << 2 | in[1] >> 4);
        out[1] = (uint8_t)(in[1] << 4 | in[2] >> 2);
        out[2] = (uint8_t)(((in[2] << 6) & 0xc0) | in[3]);
    }
}

//
// Base-32
//

/// Compute the maximum size in bytes that an array of bytes of length inputSize would be encoded as in
/// Base-32 encoding. If lineLength is not zero, then the number of lines that would be output is also computed,
/// and the size of the newlines is added to the result.
PRIME_PUBLIC size_t Base32ComputeMaxEncodedSize(size_t inputSize, int lineLength = 0, size_t newlineSize = 0);

/// Base-32 encode sourceSize bytes from the memory pointed to by source in to the buffer specified by buffer.
/// For every lineLength bytes of output, a newline will be written to the output (lineLength can be zero to
/// disable newlines). Returns the number of bytes written to the buffer. If the buffer is a NULL pointer, nothing
/// is written, but the function returns the number of bytes that would be written.
/// DOES NOT NULL TERMINATE THE OUTPUT.
PRIME_PUBLIC size_t Base32Encode(void* buffer, size_t bufferSize, const void* source, size_t sourceSize,
    int lineLength, StringView newline = "");

/// Decode Base-32 encoded data of sourceSize bytes in length from the memory pointed to by source in to the
/// memory pointed to by buffer. Returns the number of bytes decoded, or -1 if the buffer is too small.
PRIME_PUBLIC ptrdiff_t Base32Decode(void* buffer, size_t bufferSize, StringView source);

/// Computes the maximum number of bytes that Base-32 input of the specified size in bytes could be decoded to.
PRIME_PUBLIC size_t Base32ComputeMaxDecodedSize(size_t inputSize);

/// Manually build the Base-32 decoding table. This is normally handled for you by Base32Decode(), but in
/// a multithreaded application calling this can prevent a race condition.
PRIME_PUBLIC void Base32BuildDecodingTable();

PRIME_PUBLIC void Base32EncodeAppend(std::string& out, const void* data, size_t size);

PRIME_PUBLIC void Base32EncodeAppend(std::string& out, StringView string);

PRIME_PUBLIC bool Base32DecodeAppend(std::string& out, StringView string);

PRIME_PUBLIC std::string Base32Encode(StringView source);

PRIME_PUBLIC std::string Base32Decode(StringView source);

//
// URL encoding
//

namespace Private {
    // 32-byte lookup table for URL safe bytes.
    PRIME_PUBLIC extern const uint8_t urlSafeLUT[256 / 8];

    // 32-byte lookup table for URL legal bytes.
    PRIME_PUBLIC extern const uint8_t urlLegalLUT[256 / 8];
}

/// Safe characters are those that are safe within a component.
inline unsigned int IsURLSafe(unsigned char uch)
{
    return Private::urlSafeLUT[uch / 8] & (1 << (uch & 7));
}

/// Legal characters are those that may appear anywhere in a URL. So '%' is legal, as is '/', but ' ' is not.
inline unsigned int IsURLLegal(unsigned char uch)
{
    return Private::urlLegalLUT[uch / 8] & (1 << (uch & 7));
}

PRIME_PUBLIC bool IsURLSafe(StringView string);
PRIME_PUBLIC bool IsURLLegal(StringView string);

enum URLEncodeFlags {
    URLEncodeFlagLeaveSpaces = 1u << 0,
    URLEncodeFlagSpacesAsPluses = 1u << 1,
};

/// If buffer is NULL, returns the encoded size without writing anything to the buffer. DOES NOT NULL-TERMINATE.
PRIME_PUBLIC size_t URLEncode(char* buffer, size_t bufferSize, StringView source, unsigned int options = 0);

enum URLDecodeFlags {
    URLDecodeFlagPlusesAsSpaces = 1u << 0,
};

/// If buffer is NULL, returns the decoded size without writing anything to the buffer. DOES NOT NULL-TERMINATE.
PRIME_PUBLIC size_t URLDecode(char* buffer, size_t bufferSize, StringView source, unsigned int options = 0);

/// options is a combination of the URLEncodeFlags
PRIME_PUBLIC std::string URLEncode(StringView source, unsigned int options = 0);

/// Appends the encoded output on to *output.
/// options is a combination of the URLEncodeFlags
PRIME_PUBLIC void URLEncodeAppend(std::string& output, StringView source, unsigned int options = 0);

/// options is a combination of the URLDecodeFlags
PRIME_PUBLIC std::string URLDecode(StringView source, unsigned int options = 0);

/// Appends the decoded output on to *output.
/// options is a combination of the URLDecodeFlags
PRIME_PUBLIC void URLDecodeAppend(std::string& output, StringView source, unsigned int options = 0);

//
// HTML escaping
//

enum HTMLEscapeFlags {
    HTMLEscapeFlagLeaveQuotes = 1u << 0,
    HTMLEscapeFlagNewlinesToBR = 1u << 1
};

/// Escapes <, >, & and optionally " and '. If buffer is NULL, returns the escaped size without writing
/// anything to the buffer. DOES NOT NULL-TERMINATE.
PRIME_PUBLIC size_t HTMLEscape(char* buffer, size_t bufferSize, StringView source, unsigned int options = 0);

/// options is a combination of the HTMLEscapeFlags
PRIME_PUBLIC std::string HTMLEscape(StringView source, unsigned int options = 0);

/// options is a combination of the HTMLEscapeFlags
PRIME_PUBLIC void HTMLEscapeAppend(std::string& output, StringView source, unsigned int options = 0);

/// Allows me to forget the name of the HTMLEscapeFlagNewlinesToBR flag.
inline std::string HTMLBREscape(StringView source, unsigned int options = 0)
{
    return HTMLEscape(source, options | HTMLEscapeFlagNewlinesToBR);
}

/// Allows me to forget the name of the HTMLEscapeFlagNewlinesToBR flag.
inline void HTMLBREscapeAppend(std::string& output, StringView source, unsigned int options = 0)
{
    HTMLEscapeAppend(output, source, options | HTMLEscapeFlagNewlinesToBR);
}

//
// HTML unescaping
//

struct HTMLEntity {
    const char* token;
    uint32_t entity;
    const char* string;
};

PRIME_PUBLIC ArrayView<const HTMLEntity> GetHTMLEntities();

/// The array will be sorted.
PRIME_PUBLIC ArrayView<const char*> GetHTMLEmptyElements();

/// Convert HTML character entities and hex escapes to UTF-8. If buffer is NULL, returns the size of the output
/// without writing anything to the buffer. DOES NOT NULL-TERMINATE.
PRIME_PUBLIC size_t HTMLUnescape(char* buffer, size_t bufferSize, StringView source, ArrayView<const HTMLEntity> entities = GetHTMLEntities());

PRIME_PUBLIC std::string HTMLUnescape(StringView source);

PRIME_PUBLIC void HTMLUnescapeAppend(std::string& output, StringView source);

//
// Converting arbitrary text to identifiers
//

/// Returns a string consisting of only letters, numbers and the characters from safe. All other characters are
/// replaced with replacement (unless replacement is zero, in which case they are removed).
PRIME_PUBLIC std::string EncodeIdentifier(StringView source, const char* safe, char replacement);

/// Returns a string consisting of only letters, numbers and the characters from safe. All other characters are
/// replaced with replacement (unless replacement is zero, in which case they are removed).
PRIME_PUBLIC std::string EncodeIdentifier(StringView string, const char* safe, char replacement);

//
// Hex encoding
//

/// Null terminates *buffer if there's room. Returns true if all the bytes were encoded and *buffer was null
/// terminated.
PRIME_PUBLIC bool HexEncode(char* buffer, size_t bufferSize, const void* bytes, size_t bytesSize);

/// Returns true if bufferSize was large enough to hold all the digits.
PRIME_PUBLIC bool HexDecode(void* buffer, size_t bufferSize, StringView digits);

/// Wrapper around HexEncode that returns a std::string.
PRIME_PUBLIC std::string HexEncode(const void* bytes, size_t bytesSize);

/// Wrapper around HexEncode that returns a std::string.
PRIME_PUBLIC std::string HexEncode(StringView string);

//
// Email addresses
//

PRIME_PUBLIC bool IsValidEmailAddress(StringView email);

PRIME_PUBLIC bool IsValidEmailAddressStrict(StringView email);

/// Returns true if email points to a probably valid email address, false if it doesn't. If the email address is
/// not strictly valid, but is well formed, returns true and sets *strictFail to true (if strictFail is not null).
PRIME_PUBLIC bool ParseEmailAddress(std::string& localPart, std::string& domain, StringView email, bool* strictFail = NULL);

//
// MIME filenames
//

/// I was hoping this was going to be using % escaping but that doesn't seem to work universally, so instead
/// it replaces characters that aren't known to be safe with underscores. Hence, there's no decode.
PRIME_PUBLIC std::string MIMEFilenameEncode(StringView input);
}

#endif
