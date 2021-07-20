// Copyright 2000-2021 Mark H. P. Lord

#include "TextEncoding.h"
#include "Convert.h"
#include "NumberUtils.h"
#include "StringUtils.h"

namespace Prime {

//
// UTF-8
//

bool UTF8Decoder::read(uint32_t& ch)
{
    if (hasFinished()) {
        return false;
    }

    unsigned int utf8Length = UTF8DecodeLengthFromLeadingByte(*_src);

    if (utf8Length > _srcLength) {
        _valid = false;
        return false;
    }

    if (utf8Length == 1) {
        ch = *_src++;
        --_srcLength;
    } else {
        if (utf8Length == 0 || !UTF8VerifyTrailingBytes(_src, utf8Length)) {
            // Skip the byte if we have an invalid sequence.
            _valid = false;
            ++_src;
            --_srcLength;
            return true;
        }

        ch = UTF8Decode(_src, utf8Length);

        _src += utf8Length;
        _srcLength -= utf8Length;

        if (_strict) {
            if (UTF8EncodedLength(ch) != utf8Length) {
                _valid = false; // ...but don't skip it (in case of bad encoders)
            }
        }
    }

    return true;
}

unsigned int UTF8EncodedLength(uint32_t ch)
{
    if (ch < 0x80)
        return 1;
    else if (ch < 0x800)
        return 2;
    else if (ch < 0x10000)
        return 3;
    else if (ch < 0x200000)
        return 4;
    else if (ch < 0x4000000)
        return 5;
    else
        return 6;
}

size_t UTF8Encode(void* destVoid, uint32_t ch)
{
    static const uint8_t utf8ByteMark = 0x80;
    static const uint8_t utf8ByteMask = 0xBF;
    static const uint8_t utf8FirstByteMark[] = { 0xff, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc };

    size_t len = UTF8EncodedLength(ch);

    uint8_t* dest = reinterpret_cast<uint8_t*>(destVoid) + len;

    switch (len) {
    case 6:
        *--dest = (uint8_t)((ch | utf8ByteMark) & utf8ByteMask);
        ch >>= 6;
    case 5:
        *--dest = (uint8_t)((ch | utf8ByteMark) & utf8ByteMask);
        ch >>= 6;
    case 4:
        *--dest = (uint8_t)((ch | utf8ByteMark) & utf8ByteMask);
        ch >>= 6;
    case 3:
        *--dest = (uint8_t)((ch | utf8ByteMark) & utf8ByteMask);
        ch >>= 6;
    case 2:
        *--dest = (uint8_t)((ch | utf8ByteMark) & utf8ByteMask);
        ch >>= 6;
    case 1:
        *--dest = (uint8_t)(ch | utf8FirstByteMark[len]);
    }

    return len;
}

unsigned int UTF8DecodeLengthFromLeadingByte(uint8_t ch)
{
    if (!(ch & PRIME_BINARY(10000000))) {
        return 1;
    } else if ((ch & PRIME_BINARY(11100000)) == PRIME_BINARY(11000000)) {
        return 2;
    } else if ((ch & PRIME_BINARY(11110000)) == PRIME_BINARY(11100000)) {
        return 3;
    } else if ((ch & PRIME_BINARY(11111000)) == PRIME_BINARY(11110000)) {
        return 4;
    } else if ((ch & PRIME_BINARY(11111100)) == PRIME_BINARY(11111000)) {
        return 5;
    } else if ((ch & PRIME_BINARY(11111110)) == PRIME_BINARY(11111100)) {
        return 6;
    }

    return 0;
}

bool UTF8VerifyTrailingBytes(const void* bytesIn, unsigned int len)
{
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(bytesIn);
    unsigned int k;

    for (k = 1; k != len; ++k) {
        if ((bytes[k] & PRIME_BINARY(11000000)) != PRIME_BINARY(10000000)) {
            return false;
        }
    }

    return true;
}

uint32_t UTF8Decode(const void* bytesIn, unsigned int len)
{
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(bytesIn);
    uint32_t c;

    PRIME_DEBUG_ASSERT(len >= 1 && len <= 6);

    switch (len) {
    default:
        return 0;

    case 1:
        return bytes[0];

    case 2:
        c = ((uint32_t)(bytes[0] & PRIME_BINARY(00011111))) << 6;
        c |= ((uint32_t)(bytes[1] & PRIME_BINARY(00111111)));
        break;
    case 3:
        c = ((uint32_t)(bytes[0] & PRIME_BINARY(00001111))) << 12;
        c |= ((uint32_t)(bytes[1] & PRIME_BINARY(00111111))) << 6;
        c |= ((uint32_t)(bytes[2] & PRIME_BINARY(00111111)));
        break;
    case 4:
        c = ((uint32_t)(bytes[0] & PRIME_BINARY(00000111))) << 18;
        c |= ((uint32_t)(bytes[1] & PRIME_BINARY(00111111))) << 12;
        c |= ((uint32_t)(bytes[2] & PRIME_BINARY(00111111))) << 6;
        c |= ((uint32_t)(bytes[3] & PRIME_BINARY(00111111)));
        break;
    case 5:
        c = ((uint32_t)(bytes[0] & PRIME_BINARY(00000011))) << 24;
        c |= ((uint32_t)(bytes[1] & PRIME_BINARY(00111111))) << 18;
        c |= ((uint32_t)(bytes[2] & PRIME_BINARY(00111111))) << 12;
        c |= ((uint32_t)(bytes[3] & PRIME_BINARY(00111111))) << 6;
        c |= ((uint32_t)(bytes[4] & PRIME_BINARY(00111111)));
        break;
    case 6:
        c = ((uint32_t)(bytes[0] & PRIME_BINARY(00000001))) << 30;
        c |= ((uint32_t)(bytes[1] & PRIME_BINARY(00111111))) << 24;
        c |= ((uint32_t)(bytes[2] & PRIME_BINARY(00111111))) << 18;
        c |= ((uint32_t)(bytes[3] & PRIME_BINARY(00111111))) << 12;
        c |= ((uint32_t)(bytes[4] & PRIME_BINARY(00111111))) << 6;
        c |= ((uint32_t)(bytes[5] & PRIME_BINARY(00111111)));
        break;
    }

    return c;
}

bool UTF8IsValid(const void* ptrIn, const void* endIn, unsigned int* len)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(ptrIn);
    const uint8_t* end = reinterpret_cast<const uint8_t*>(endIn);

    if (ptr == end) {
        return false;
    }

    unsigned int dl = UTF8DecodeLengthFromLeadingByte(*ptr);
    if (dl == 0) {
        return false;
    }

    if (end - ptr < (ptrdiff_t)dl) {
        return false;
    }

    if (!UTF8VerifyTrailingBytes(ptr, dl)) {
        return false;
    }

    if (len) {
        *len = dl;
    }

    return true;
}

StringView::size_type UTF8FindCodePoint(StringView string, uint32_t codepoint)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(string.begin());
    const uint8_t* end = reinterpret_cast<const uint8_t*>(string.end());

    if (codepoint < 0x80) {
        while (ptr != end) {
            if (*ptr >= 0x80) {
                ++ptr;
                continue;
            }

            if (*ptr == codepoint) {
                return static_cast<StringView::size_type>(ptr - reinterpret_cast<const uint8_t*>(string.begin()));
            }

            ++ptr;
        }

    } else {
        while (ptr != end) {
            if (*ptr < 0x80) {
                ++ptr;
                continue;
            }

            unsigned int length = UTF8DecodeLengthFromLeadingByte(*ptr);
            if (length == 0) {
                ++ptr;
                continue;
            }

            if (end - ptr < static_cast<int>(length)) {
                return StringView::npos;
            }

            uint32_t codepoint2 = UTF8Decode(ptr, length);

            if (codepoint2 == codepoint) {
                return static_cast<StringView::size_type>(ptr - reinterpret_cast<const uint8_t*>(string.begin()));
            }

            ptr += length;
        }
    }

    return StringView::npos;
}

const char* UTF8ReverseFindLeadingByte(const void* ptr, const void* begin)
{
    const uint8_t* ptr8 = reinterpret_cast<const uint8_t*>(ptr);
    const uint8_t* begin8 = reinterpret_cast<const uint8_t*>(begin);

    while (ptr8 != begin8) {
        --ptr8;
        if ((*ptr8 & 0xc0) == 0xc0) {
            return reinterpret_cast<const char*>(ptr8);
        }
        if ((*ptr8 & 0x80) == 0) {
            break;
        }
    }

    return NULL;
}

StringView::size_type UTF8FindLastOf(StringView string, StringView chars)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(string.end());
    const uint8_t* begin = reinterpret_cast<const uint8_t*>(string.begin());

    for (;;) {
        if (ptr == begin) {
            return StringView::npos;
        }

        --ptr;

        if (*ptr & 0x80) {
            if ((*ptr & 0xc0) == 0x80) {
                // This is a trailing byte, find the leading byte and make sure the length checks out
                const uint8_t* start = reinterpret_cast<const uint8_t*>(UTF8ReverseFindLeadingByte(ptr, begin));
                if (!start) {
                    // Couldn't find the leading byte, so this is invalid - no match to chars
                    break;
                }

                unsigned int length = UTF8DecodeLengthFromLeadingByte(*start);
                if (length != static_cast<size_t>(ptr - start) + 1u) {
                    // We have some unaccounted for trailing bytes - no match to chars
                    break;
                }

                uint32_t codepoint = UTF8Decode(start, length);

                ptr = start;

                if (UTF8FindCodePoint(chars, codepoint) != StringView::npos) {
                    break;
                }

            } else {
                // This is a leading byte without a trailing byte and therefore invalid - no match to chars
                break;
            }

        } else {
            if (UTF8FindCodePoint(chars, *ptr) != StringView::npos) {
                break;
            }
        }
    }

    return static_cast<StringView::size_type>(ptr - begin);
}

StringView::size_type UTF8FindFirstOf(StringView string, StringView chars)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(string.begin());
    const uint8_t* end = reinterpret_cast<const uint8_t*>(string.end());

    while (ptr != end) {
        if (*ptr & 0x80) {
            unsigned int length = UTF8DecodeLengthFromLeadingByte(*ptr);
            if (length == 0) {
                ++ptr;
                continue;
            }

            if (end - ptr < static_cast<int>(length)) {
                ptr = end;
                break;
            }

            uint32_t codepoint = UTF8Decode(ptr, length);

            if (UTF8FindCodePoint(chars, codepoint) != StringView::npos) {
                break;
            }

            ptr += length;

        } else {
            if (UTF8FindCodePoint(chars, *ptr) != StringView::npos) {
                break;
            }

            ++ptr;
        }
    }

    return ptr == end ? StringView::npos : static_cast<StringView::size_type>(ptr - reinterpret_cast<const uint8_t*>(string.begin()));
}

StringView::size_type UTF8FindLastNotOf(StringView string, StringView chars)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(string.end());
    const uint8_t* begin = reinterpret_cast<const uint8_t*>(string.begin());

    for (;;) {
        if (ptr == begin) {
            return StringView::npos;
        }

        --ptr;

        if (*ptr & 0x80) {
            if ((*ptr & 0xc0) == 0x80) {
                // This is a trailing byte, find the leading byte and make sure the length checks out
                const uint8_t* start = reinterpret_cast<const uint8_t*>(UTF8ReverseFindLeadingByte(ptr, begin));
                if (!start) {
                    // Couldn't find the leading byte, so this is invalid - no match to chars
                    continue;
                }

                unsigned int length = UTF8DecodeLengthFromLeadingByte(*start);
                if (length != static_cast<size_t>(ptr - start) + 1u) {
                    // We have some unaccounted for trailing bytes - no match to chars
                    continue;
                }

                uint32_t codepoint = UTF8Decode(start, length);

                ptr = start;

                if (UTF8FindCodePoint(chars, codepoint) == StringView::npos) {
                    break;
                }

            } else {
                // This is a leading byte without a trailing byte and therefore invalid - no match to chars
                continue;
            }

        } else {
            if (UTF8FindCodePoint(chars, *ptr) == StringView::npos) {
                break;
            }
        }
    }

    return static_cast<StringView::size_type>(ptr - begin);
}

StringView::size_type UTF8FindFirstNotOf(StringView string, StringView chars)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(string.begin());
    const uint8_t* end = reinterpret_cast<const uint8_t*>(string.end());

    while (ptr != end) {
        if (*ptr & 0x80) {
            unsigned int length = UTF8DecodeLengthFromLeadingByte(*ptr);
            if (length == 0) {
                break;
            }

            if (end - ptr < static_cast<int>(length)) {
                break;
            }

            uint32_t codepoint = UTF8Decode(ptr, length);

            if (UTF8FindCodePoint(chars, codepoint) == StringView::npos) {
                break;
            }

            ptr += length;

        } else {
            if (UTF8FindCodePoint(chars, *ptr) == StringView::npos) {
                break;
            }

            ++ptr;
        }
    }

    return ptr == end ? StringView::npos : static_cast<StringView::size_type>(ptr - reinterpret_cast<const uint8_t*>(string.begin()));
}

const char* UTF8Advance(const void* ptr, const void* end, size_t numberOfCodepoints)
{
    const uint8_t* ptr8 = reinterpret_cast<const uint8_t*>(ptr);
    const uint8_t* end8 = reinterpret_cast<const uint8_t*>(end);

    for (; ptr8 != end8 && numberOfCodepoints != 0; --numberOfCodepoints) {

        if (*ptr8 & 0x80) {

            if ((*ptr8 & 0xc0) == 0xc0) {

                unsigned int length = UTF8DecodeLengthFromLeadingByte(*ptr8);

                if (length > static_cast<size_t>(end8 - ptr8)) {
                    // This character won't fit, skip to the end
                    return reinterpret_cast<const char*>(end8);
                }

                // TODO: security risk if it isn't followed by trailing bytes?

                ptr8 += length;

            } else {

                // Stray trailing byte, we'll have to treat it as ISO-8859-1
                ++ptr8;
            }

        } else {

            ++ptr8;
        }
    }

    return reinterpret_cast<const char*>(ptr8);
}

StringView UTF8Advance(StringView string, size_t numberOfCodepoints)
{
    return StringView(reinterpret_cast<const char*>(UTF8Advance(string.begin(), string.end(), numberOfCodepoints)), string.end());
}

//
// UTF-16
//

bool UTF16Decoder::read(uint32_t& ch)
{
    PRIME_ASSERT(_src <= _srcEnd);
    if (hasFinished()) {
        return false;
    }

    static const uint16_t utf16SurrogateMask = 0x3ffu;
    static const unsigned int utf16SurrogateShift = 10;
    static const uint32_t utf16SurrogateAdd = 0x0010000u;

    ch = *_src++;

    if (!UTF16IsLeadingWord(ch)) {
        return true;
    }

    if (_src == _srcEnd) {
        _valid = false; // trailing word missing from end
        return false;
    } else {
        unsigned int ch2 = *_src;
        if (!UTF16IsTrailingWord(ch2)) {
            _valid = false; // trailing word is invalid
            return true; // return the leading word
        } else {
            ++_src;

            ch = (ch & utf16SurrogateMask) << utf16SurrogateShift;
            ch += (ch2 & utf16SurrogateMask);
            ch += utf16SurrogateAdd;
        }
    }

    return true;
}

void UTF16ByteSwap(uint16_t* buffer, size_t len)
{
    uint16_t* end;

    end = buffer + len;
    for (; buffer != end; ++buffer) {
        *buffer = ((*buffer >> 8) & 0xff) | ((*buffer << 8) & 0xff00);
    }
}

unsigned int UTF16EncodedLength(uint32_t ch)
{
    PRIME_DEBUG_ASSERT(UTF16CanEncode(ch));

    return (ch >= 0x10000) ? 2 : 1;
}

unsigned int UTF16Encode(uint16_t* dest, uint32_t ch)
{
    PRIME_DEBUG_ASSERT(UTF16CanEncode(ch));

    if (ch >= 0x10000) {
        uint32_t v = ch - 0x10000;
        unsigned int vh = (v >> 10);
        unsigned int vl = (v & 0x3ff);
        unsigned int w1 = (0xd800 | vh);
        unsigned int w2 = (0xdc00 | vl);

        dest[0] = (uint16_t)w1;
        dest[1] = (uint16_t)w2;

        return 2;
    } else {
        *dest = (uint16_t)ch;

        return 1;
    }
}

//
// Unicode conversion
//

size_t UTF16ToUTF8(const uint16_t* src, size_t srcLength, void* destVoid, bool* isValid)
{
    uint8_t dummyDest[8];
    uint8_t* destPtr;
    uint32_t ch;

    UTF16Decoder decoder(src, srcLength);

    uint8_t* dest = reinterpret_cast<uint8_t*>(destVoid);
    destPtr = dest;

    while (decoder.read(ch)) {
        destPtr += UTF8Encode(dest ? destPtr : dummyDest, ch);
    }

    PRIME_DEBUG_ASSERT(!dest || (ptrdiff_t)UTF16ToUTF8(src, srcLength, 0, 0) == destPtr - dest);

    if (isValid) {
        *isValid = decoder.wasValid();
    }

    return destPtr - dest;
}

size_t UTF16ToUCS4(const uint16_t* src, size_t srcLength, uint32_t* dest, bool* isValid)
{
    uint32_t* destPtr;
    uint32_t ch;

    UTF16Decoder decoder(src, srcLength);

    destPtr = dest;

    while (decoder.read(ch)) {
        if (dest) {
            *destPtr = ch;
        }

        destPtr++;
    }

    PRIME_DEBUG_ASSERT(!dest || (ptrdiff_t)UTF16ToUCS4(src, srcLength, 0, 0) == destPtr - dest);

    if (isValid) {
        *isValid = decoder.wasValid();
    }

    return destPtr - dest;
}

size_t UTF8ToUTF16(const void* src, size_t srcLength, uint16_t* dest, bool* isValid)
{
    uint16_t* destPtr = dest;
    uint16_t dummyDest[2];
    uint32_t ch = 0;

    UTF8Decoder decoder(src, srcLength, isValid != 0);

    while (decoder.read(ch)) {
        destPtr += UTF16Encode(dest ? destPtr : dummyDest, ch);
    }

    PRIME_DEBUG_ASSERT(!dest || (ptrdiff_t)UTF8ToUTF16(src, srcLength, 0, 0) == destPtr - dest);

    if (isValid) {
        *isValid = decoder.wasValid();
    }

    return destPtr - dest;
}

size_t UTF8ToUCS4(const void* src, size_t srcLength, uint32_t* dest, bool* isValid)
{
    uint32_t* destPtr = dest;
    uint32_t ch = 0;

    UTF8Decoder decoder(src, srcLength, isValid != 0);

    while (decoder.read(ch)) {
        if (dest) {
            *destPtr = ch;
        }

        ++destPtr;
    }

    PRIME_DEBUG_ASSERT(!dest || (ptrdiff_t)UTF8ToUCS4(src, srcLength, 0, 0) == destPtr - dest);

    if (isValid) {
        *isValid = decoder.wasValid();
    }

    return destPtr - dest;
}

size_t UCS4ToUTF8(const uint32_t* src, size_t srcLength, void* destVoid, bool* isValid)
{
    const uint32_t* srcEnd;
    uint8_t dummyDest[8];
    uint8_t* destPtr;
    bool isValidDummy;

    if (!isValid) {
        isValid = &isValidDummy;
    }

    *isValid = true;

    srcEnd = src + srcLength;
    uint8_t* dest = reinterpret_cast<uint8_t*>(destVoid);
    destPtr = dest;

    while (src != srcEnd) {
        destPtr += UTF8Encode(dest ? destPtr : dummyDest, *src++);
    }

    return destPtr - dest;
}

size_t UCS4ToUTF16(const uint32_t* src, size_t srcLength, uint16_t* dest, bool* isValid)
{
    uint16_t* destPtr = dest;
    const uint32_t* srcEnd = src + srcLength;
    uint16_t dummyDest[2];
    bool isValidDummy;

    if (!isValid) {
        isValid = &isValidDummy;
    }

    *isValid = true;

    while (src != srcEnd) {
        uint32_t ch = *src++;

        if (!UTF16CanEncode(ch)) {
            *isValid = false;
            continue;
        }

        destPtr += UTF16Encode(dest ? destPtr : dummyDest, ch);
    }

    return destPtr - dest;
}

//
// Unicode information
//

namespace Private {
    bool IsUnicodeWhitespace2(uint32_t codepoint)
    {
        uint32_t high = codepoint >> 8;
        uint32_t low = codepoint & 0x0f;

        PRIME_DEBUG_ASSERT(high != 0); // IsUnicodeWhitespace should have dealt with this

        if (high == 0x20) {
            return low <= 0x0a || low == 0x28 || low == 0x29 || low == 0x2f || low == 0x5f;
        }

        // U+180e and U+feff used to be classified as whitespace but now aren't
        return codepoint == 0x1680 || codepoint == 0x180e || codepoint == 0x3000 || codepoint == 0xfeff;
    }
}

//
// CEscape/CUnescape
//

size_t CEscape(char* buffer, size_t bufferSize, StringView src, unsigned int flags)
{
    const unsigned char* usrc = (const unsigned char*)src.begin();
    const unsigned char* usrcEnd = usrc + src.size();
    char* dest = buffer;
    char* destEnd;

    if (flags & (CEscapeFlagKeepUTF8 | CEscapeFlagEscapeUTF8)) {
        flags |= CEscapeFlagUnicode;
    }

    if (buffer) {
        PRIME_ASSERT(bufferSize != 0);

        destEnd = dest + (bufferSize - 1);
    } else {
        destEnd = 0;
    }

    char escapeBuffer[16];
    const char* escaped = NULL; // Moved outside the block to silence a warning.
    for (; usrc != usrcEnd; ++usrc) {
        unsigned int escapedLength = 1;

        switch (*usrc) {
        case '\\':
            escaped = "\\\\";
            escapedLength = 2;
            break;

        case '"':
            if (flags & CEscapeFlagDoubleQuotes) {
                escaped = "\\\"";
                escapedLength = 2;
            }
            break;

        case '\'':
            if (flags & CEscapeFlagApostrophes) {
                escaped = "\\'";
                escapedLength = 2;
            }
            break;

        case ' ':
            if (flags & CEscapeFlagSpaces) {
                escaped = "\\ ";
                escapedLength = 2;
            }
            break;

        case '?':
            if (flags & CEscapeFlagQuestionMarks) {
                escaped = "\\ ";
                escapedLength = 2;
            }
            break;

        case '*':
            if (flags & CEscapeFlagAsterisks) {
                escaped = "\\ ";
                escapedLength = 2;
            }
            break;

        case '\0':
            if (!(flags & CEscapeFlagStripNulls)) {
                escaped = "\\0";
                escapedLength = 2;
            } else {
                escapedLength = 0;
            }
            break;

        case '\a':
            if (flags & CEscapeFlagA) {
                escaped = "\\a";
                escapedLength = 2;
            }
            break;

        case '\b':
            if (flags & CEscapeFlagB) {
                escaped = "\\b";
                escapedLength = 2;
            }
            break;

        case '\f':
            if (flags & CEscapeFlagF) {
                escaped = "\\f";
                escapedLength = 2;
            }
            break;

        case '\n':
            if (flags & CEscapeFlagN) {
                escaped = "\\n";
                escapedLength = 2;
            }
            break;

        case '\r':
            if (flags & CEscapeFlagR) {
                escaped = "\\r";
                escapedLength = 2;
            }
            break;

        case '\t':
            if (flags & CEscapeFlagT) {
                escaped = "\\t";
                escapedLength = 2;
            }
            break;

        case '\v':
            if (flags & CEscapeFlagV) {
                escaped = "\\v";
                escapedLength = 2;
            }
            break;

        default:
            if (!(flags & CEscapeFlagNoHex) && (*usrc < ' ' || *usrc >= 0x7f)) {
                if (*usrc >= 0x80) {
                    unsigned int utf8Length = UTF8DecodeLengthFromLeadingByte(*usrc);

                    if (!utf8Length) {
                        // We have a UTF-8 continuation byte. Skip it.
                        escapedLength = 0;

                    } else if (usrcEnd - usrc >= (ptrdiff_t)utf8Length) {
                        uint32_t unicode = UTF8Decode(usrc, utf8Length);

                        if (flags & CEscapeFlagEscapeUTF8) {
                            usrc += (utf8Length - 1);

                            // TODO: transcode to UTF-16
                            StringFormat(escapeBuffer, sizeof(escapeBuffer), "\\u%04" PRIx32, unicode);
                            escapedLength = 6;
                            escaped = escapeBuffer;
                            break;
                        }

                        if (flags & CEscapeFlagKeepUTF8) {
                            // Certain codepoints need escaping regardless due to issue inside HTML files.
                            if (unicode == 0x2028 || unicode == 0x2029) {
                                usrc += (utf8Length - 1);

                                // TODO: transcode to UTF-16
                                StringFormat(escapeBuffer, sizeof(escapeBuffer), "\\u%04" PRIx32, unicode);
                                escapedLength = 6;
                                escaped = escapeBuffer;
                            } else {
                                escaped = (const char*)usrc;
                                escapedLength = utf8Length;
                                usrc += (utf8Length - 1);
                            }
                            break;
                        }
                    }
                }

                if (flags & CEscapeFlagUnicode) {
                    StringFormat(escapeBuffer, sizeof(escapeBuffer), "\\u00%02x", (unsigned int)*usrc);
                    escaped = escapeBuffer;
                    escapedLength = 6;
                } else {
                    StringFormat(escapeBuffer, sizeof(escapeBuffer), "\\x%02x", (unsigned int)*usrc);
                    escaped = escapeBuffer;
                    escapedLength = 4;
                }
            }
            break;
        }

        if (escapedLength == 1) {
            if (dest < destEnd) {
                *dest = (char)*usrc;
            }

            ++dest;
        } else {
            while (escapedLength--) {
                if (dest < destEnd) {
                    *dest = *escaped++;
                }

                ++dest;
            }
        }
    }

    if (buffer) {
        if (dest <= destEnd) {
            *dest = 0;
        } else {
            buffer[bufferSize - 1] = 0;
        }
    }

    return dest - buffer;
}

size_t CUnescape(char* buffer, size_t bufferSize, StringView src)
{
    const char* ptr = src.begin();
    const char* end = src.end();
    char* dest = buffer;
    ptrdiff_t destSpace;

    if (buffer) {
        PRIME_ASSERT(bufferSize != 0);

        destSpace = (ptrdiff_t)(bufferSize - 1);
    } else {
        destSpace = 0;
    }

    for (;;) {
        const char* begin = ptr;
        while (ptr != end && *ptr != '\\') {
            ++ptr;
        }

        if (ptr != begin) {
            ptrdiff_t appendLength = ptr - begin;
            if (appendLength > destSpace) {
                appendLength = destSpace;
            }
            if (appendLength) {
                memcpy(dest, begin, appendLength);
            }
            destSpace -= appendLength;
            dest += ptr - begin;
        }

        if (ptr == end) {
            break;
        }

        // Skip the backslash and decode the character.
        ++ptr;

        char decoded[20];
        ptrdiff_t decodedLength = 1;

        switch (*ptr) {
        case '0': {
            int n = 0;
            ParseOctInt(StringView(ptr, end).substr(0, 4), ptr, n);
            decoded[0] = (char)n;
            break;
        }
        case 'a':
            decoded[0] = '\a';
            ++ptr;
            break;
        case 'b':
            decoded[0] = '\b';
            ++ptr;
            break;
        case 'f':
            decoded[0] = '\f';
            ++ptr;
            break;
        case 'n':
            decoded[0] = '\n';
            ++ptr;
            break;
        case 'r':
            decoded[0] = '\r';
            ++ptr;
            break;
        case 't':
            decoded[0] = '\t';
            ++ptr;
            break;
        case 'v':
            decoded[0] = '\v';
            ++ptr;
            break;
        case 'u':
        case 'U':
        case 'x':
        case 'X': {
            const char* newPtr;
            uint32_t unicode;
            int maxDig = (*ptr == 'x' || *ptr == 'X') ? 2 : 4;
            maxDig = Narrow<int>(Min<ptrdiff_t>(maxDig, (end - ptr) - 1));
            if (maxDig && ParseHexInt(StringView(ptr + 1, end).substr(0, maxDig), newPtr, unicode)) {
                decodedLength = UTF8Encode(decoded, unicode);
                ptr = newPtr;
            } else {
                decoded[0] = *ptr++;
            }
            break;
        }
        default:
            decoded[0] = *ptr++;
            break;
        }

#if 1
        // Never write a partial UTF-8 character.
        if (decodedLength <= destSpace) {
            memcpy(dest, decoded, decodedLength);
            destSpace -= decodedLength;
        } else {
            memset(dest, '.', destSpace);
            destSpace = 0;
        }

        dest += decodedLength;
#else
        ptrdiff_t appendLength = decodedLength;
        if (appendLength > destSpace) {
            appendLength = destSpace;
        }
        memcpy(dest, decoded, appendLength);
        destSpace -= appendLength;
        dest += decodedLength;
#endif
    }

    if (buffer) {
        *dest = 0;
    }

    return (size_t)(dest - buffer);
}

//
// Base-64
//

namespace Base64 {

    const uint8_t encodingTable[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    uint8_t decodingTable[256];

    void BuildDecodingTable()
    {
        static bool builtDecodingTable = false;
        if (builtDecodingTable) {
            return;
        }

        memset(decodingTable, decodingTableInvalidChar, sizeof(decodingTable));

        uint8_t n = 0;
        for (const uint8_t* p = encodingTable; p != encodingTable + 64; ++p, ++n) {
            decodingTable[*p] = n;
        }

        decodingTable[padChar] = decodingTablePadChar;

        builtDecodingTable = true;
    }
}

size_t Base64ComputeMaxEncodedSize(size_t inputSize, int lineLength, size_t newlineSize)
{
    size_t encodedSize = (inputSize + 2) / 3 * 4;

    if (lineLength) {
        size_t lineCount = (encodedSize + lineLength - 1) / lineLength;
        return encodedSize + lineCount * newlineSize;
    }

    return encodedSize;
}

std::string Base64Encode(const void* source, size_t sourceSize, int lineLength, StringView newline)
{
    std::string result;

    size_t size = Base64Encode(NULL, 0, source, sourceSize, lineLength, newline);

    result.resize(size);

    Base64Encode(&result[0], size, source, sourceSize, lineLength, newline);

    return result;
}

size_t Base64Encode(void* buffer, size_t bufferSize, const void* source, size_t sourceSize, int lineLength, StringView newline)
{
    const uint8_t* sourcePtr = (const uint8_t*)source;
    uint8_t* destPtr = (uint8_t*)buffer;
    uint8_t* destEnd = buffer ? ((uint8_t*)buffer + bufferSize) : (uint8_t*)NULL;

    int lineRemaining = lineLength ? lineLength : INT_MAX;

    while (sourceSize) {
        // Only put whole blocks on a line.
        if (lineLength && lineRemaining < 4) {
            for (size_t i = 0; i != newline.size(); ++i) {
                if (destPtr < destEnd) {
                    *destPtr = newline[i];
                }

                ++destPtr;
            }

            lineRemaining = lineLength;
        }

        uint8_t encodedBuffer[4];
        uint8_t* encoded = (destPtr + 4 < destEnd) ? destPtr : encodedBuffer;

        if (sourceSize >= 3) {
            Base64::EncodeBlock(encoded, sourcePtr);
            sourcePtr += 3;
            sourceSize -= 3;
        } else {
            uint8_t block[3];
            block[1] = block[2] = 0;
            memcpy(block, sourcePtr, sourceSize);
            Base64::EncodeBlock(encoded, block, (unsigned int)sourceSize);
            sourceSize = 0;
        }

        if (encoded == encodedBuffer) {
            for (size_t i = 0; i != 4; ++i) {
                if (destPtr < destEnd) {
                    *destPtr = encodedBuffer[i];
                }

                ++destPtr;
            }
        } else {
            destPtr += 4;
        }

        lineRemaining -= 4;
    }

    return (size_t)(destPtr - (uint8_t*)buffer);
}

size_t Base64ComputeMaxDecodedSize(size_t inputSize)
{
    return (inputSize + 3) / 4 * 3;
}

void Base64BuildDecodingTable()
{
    Base64::BuildDecodingTable();
}

ptrdiff_t Base64Decode(void* buffer, size_t bufferSize, StringView source)
{
    const uint8_t* ptr = (const uint8_t*)source.begin();
    const uint8_t* end = (const uint8_t*)source.end();

    size_t destRemaining = bufferSize;
    uint8_t* dest = (uint8_t*)buffer;

    Base64BuildDecodingTable();

    while (ptr != end) {
        int equalsCount;
        uint8_t encodedChunk[4];
        uint8_t* encodedChunkPtr;
        uint8_t decodedChunk[3];
        unsigned int decodedChunkSize;
        const uint8_t* copy;

        for (encodedChunkPtr = encodedChunk, equalsCount = 0; encodedChunkPtr != encodedChunk + 4;) {
            if (ptr != end) {
                uint8_t decoded = Base64::decodingTable[*ptr++];
                if (decoded == Base64::decodingTableInvalidChar) {
                    // skip duff input (whitespace usually).
                    continue;
                }

                if (decoded == Base64::decodingTablePadChar) {
                    decoded = 0;
                    ++equalsCount;
                }

                *encodedChunkPtr++ = decoded;
            } else {
                *encodedChunkPtr++ = 0;
                ++equalsCount;
            }
        }

        // Treat 4 padding chars as end of input.
        if (equalsCount == 4) {
            break;
        }

        PRIME_ASSERT(equalsCount >= 0 && equalsCount <= 3);

        Base64::DecodeBlock(decodedChunk, encodedChunk);

        decodedChunkSize = 3 - equalsCount;
        if (destRemaining < decodedChunkSize) {
            return -1;
        }

        destRemaining -= decodedChunkSize;
        copy = decodedChunk;

        switch (decodedChunkSize) {
        case 3:
            *dest++ = *copy++;
        case 2:
            *dest++ = *copy++;
        case 1:
            *dest++ = *copy;
        }
    }

    PRIME_ASSERT(dest == (uint8_t*)buffer + bufferSize - destRemaining);

    return dest - (uint8_t*)buffer;
}

void Base64EncodeAppend(std::string& out, const void* data, size_t dataSize)
{
    size_t maxBase64Size = Base64ComputeMaxEncodedSize(dataSize, 0, 0);

    size_t sizeWas = out.size();

    out.resize(sizeWas + maxBase64Size);

    char* ptr = &out[sizeWas];

    size_t encodedSize = Base64Encode(ptr, maxBase64Size, data, dataSize, 0);
    PRIME_ASSERT(encodedSize <= maxBase64Size);

    out.resize(sizeWas + encodedSize);
}

void Base64EncodeAppend(std::string& out, StringView source)
{
    Base64EncodeAppend(out, source.data(), source.size());
}

bool Base64DecodeAppend(std::string& out, StringView string)
{
    size_t maxDecodedSize = Base64ComputeMaxDecodedSize(string.size());

    size_t sizeWas = out.size();

    out.resize(out.size() + maxDecodedSize + 1);

    ptrdiff_t decodedSize = Base64Decode(&out[0], out.size(), string);

    PRIME_ASSERT(decodedSize != -1 && decodedSize <= (ptrdiff_t)maxDecodedSize);
    if (decodedSize < 0) {
        out.resize(sizeWas);
        return false;
    }

    out.resize(sizeWas + decodedSize);
    return true;
}

std::string Base64Encode(StringView source)
{
    std::string string;
    Base64EncodeAppend(string, source);
    return string;
}

std::string Base64Decode(StringView source)
{
    std::string string;
    Base64DecodeAppend(string, source);
    return string;
}

//
// Base-32
//

namespace Base32 {

    const uint8_t encodingTable[33] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

    const uint8_t padChar = '=';
    const uint8_t decodingTableInvalidChar = 0xff;
    const uint8_t decodingTablePadChar = 0xfe;

    uint8_t decodingTable[256];

    static void BuildDecodingTable()
    {
        static bool builtDecodingTable = false;
        if (builtDecodingTable) {
            return;
        }

        memset(decodingTable, decodingTableInvalidChar, sizeof(decodingTable));

        uint8_t n = 0;
        for (const uint8_t* p = encodingTable; p != encodingTable + 32; ++p, ++n) {
            decodingTable[*p] = n;
        }

        decodingTable[padChar] = decodingTablePadChar;

        builtDecodingTable = true;
    }

    inline void EncodeBlock(uint8_t out[8], const uint8_t in[5])
    {
        // 0: [0]top 5
        // 1: [0]bottom 3 [1]top 2
        // 2: [1]mid 5
        // 3: [1]bottom 1 [2]top 4
        // 4: [2]bottom 4 [3]top 1
        // 5: [3]mid 5
        // 6: [3]bottom 2 [4]top 3
        // 7: [4]bottom 5
        out[0] = encodingTable[in[0] >> 3];
        out[1] = encodingTable[((in[0] & PRIME_BINARY(00000111)) << 2) | ((in[1] & PRIME_BINARY(11000000)) >> 6)];
        out[2] = encodingTable[((in[1] & PRIME_BINARY(00111110)) >> 1)];
        out[3] = encodingTable[((in[1] & PRIME_BINARY(00000001)) << 4) | ((in[2] & PRIME_BINARY(11110000)) >> 4)];
        out[4] = encodingTable[((in[2] & PRIME_BINARY(00001111)) << 1) | ((in[3] & PRIME_BINARY(10000000)) >> 7)];
        out[5] = encodingTable[((in[3] & PRIME_BINARY(01111100)) >> 2)];
        out[6] = encodingTable[((in[3] & PRIME_BINARY(00000011)) << 3) | ((in[4] & PRIME_BINARY(11100000)) >> 5)];
        out[7] = encodingTable[in[4] & PRIME_BINARY(00011111)];
    }

    inline void DecodeBlock(uint8_t* out, const uint8_t* in)
    {
        out[0] = (uint8_t)((in[0] << 3) | (in[1] >> 2));
        out[1] = (uint8_t)(((in[1] & PRIME_BINARY(00000011)) << 6) | ((in[2]) << 1) | ((in[3] & PRIME_BINARY(00010000)) >> 4));
        out[2] = (uint8_t)(((in[3] & PRIME_BINARY(00001111)) << 4) | ((in[4]) >> 1));
        out[3] = (uint8_t)(((in[4] & PRIME_BINARY(00000001)) << 7) | ((in[5]) << 2) | ((in[6] & PRIME_BINARY(00011000)) >> 3));
        out[4] = (uint8_t)(((in[6] & PRIME_BINARY(00000111)) << 5) | ((in[7])));
    }

    inline void EncodeBlock(uint8_t out[8], const uint8_t in[], unsigned int inLength)
    {
        switch (inLength) {
        case 5:
            EncodeBlock(out, in);
            break;

        case 4:
            PRIME_DEBUG_ASSERT(in[4] == 0);
            EncodeBlock(out, in);
            out[7] = padChar;
            break;

        case 3:
            PRIME_DEBUG_ASSERT(in[4] == 0);
            PRIME_DEBUG_ASSERT(in[3] == 0);
            EncodeBlock(out, in);
            out[7] = padChar;
            out[6] = padChar;
            out[5] = padChar;
            break;

        case 2:
            PRIME_DEBUG_ASSERT(in[4] == 0);
            PRIME_DEBUG_ASSERT(in[3] == 0);
            PRIME_DEBUG_ASSERT(in[2] == 0);
            EncodeBlock(out, in);
            out[7] = padChar;
            out[6] = padChar;
            out[5] = padChar;
            out[4] = padChar;
            break;

        case 1:
            PRIME_DEBUG_ASSERT(in[4] == 0);
            PRIME_DEBUG_ASSERT(in[3] == 0);
            PRIME_DEBUG_ASSERT(in[2] == 0);
            PRIME_DEBUG_ASSERT(in[1] == 0);
            EncodeBlock(out, in);
            out[7] = padChar;
            out[6] = padChar;
            out[5] = padChar;
            out[4] = padChar;
            out[3] = padChar;
            out[2] = padChar;
            break;

        default:
            PRIME_DEBUG_ASSERT(0);
            break;
        }
    }
}

size_t Base32ComputeMaxEncodedSize(size_t inputSize, int lineLength, size_t newlineSize)
{
    size_t encodedSize = (inputSize + 4) / 5 * 8;

    if (lineLength) {
        size_t lineCount = (encodedSize + lineLength - 1) / lineLength;
        return encodedSize + lineCount * newlineSize;
    }

    return encodedSize;
}

size_t Base32Encode(void* buffer, size_t bufferSize, const void* source, size_t sourceSize, int lineLength, StringView newline)
{
    const uint8_t* sourcePtr = (const uint8_t*)source;
    uint8_t* destPtr = (uint8_t*)buffer;
    uint8_t* destEnd = buffer ? ((uint8_t*)buffer + bufferSize) : (uint8_t*)NULL;

    int lineRemaining = lineLength ? lineLength : INT_MAX;

    while (sourceSize) {
        // Only put whole blocks on a line.
        if (lineLength && lineRemaining < 8) {
            for (size_t i = 0; i != newline.size(); ++i) {
                if (destPtr < destEnd) {
                    *destPtr = newline[i];
                }

                ++destPtr;
            }

            lineRemaining = lineLength;
        }

        uint8_t encodedBuffer[8];
        uint8_t* encoded = (destPtr + 8 < destEnd) ? destPtr : encodedBuffer;

        if (sourceSize >= 5) {
            Base32::EncodeBlock(encoded, sourcePtr);
            sourcePtr += 5;
            sourceSize -= 5;
        } else {
            uint8_t block[5];
            memset(block, 0, sizeof(block));
            memcpy(block, sourcePtr, sourceSize);
            Base32::EncodeBlock(encoded, block, (unsigned int)sourceSize);
            sourceSize = 0;
        }

        if (encoded == encodedBuffer) {
            for (size_t i = 0; i != 8; ++i) {
                if (destPtr < destEnd) {
                    *destPtr = encodedBuffer[i];
                }

                ++destPtr;
            }
        } else {
            destPtr += 8;
        }

        lineRemaining -= 8;
    }

    return (size_t)(destPtr - (uint8_t*)buffer);
}

ptrdiff_t Base32Decode(void* buffer, size_t bufferSize, StringView source)
{
    const uint8_t* ptr = (const uint8_t*)source.begin();
    const uint8_t* end = (const uint8_t*)source.end();

    size_t destRemaining = bufferSize;
    uint8_t* dest = (uint8_t*)buffer;

    Base32::BuildDecodingTable();

    while (ptr != end) {
        int equalsCount;
        uint8_t encodedChunk[8];
        uint8_t* encodedChunkPtr;
        uint8_t decodedChunk[5];
        unsigned int decodedChunkSize;
        const uint8_t* copy;

        for (encodedChunkPtr = encodedChunk, equalsCount = 0; encodedChunkPtr != encodedChunk + 8;) {
            if (ptr != end) {
                uint8_t decoded = Base32::decodingTable[*ptr++];
                if (decoded == Base32::decodingTableInvalidChar) {
                    // skip duff input (whitespace usually).
                    continue;
                }

                if (decoded == Base32::decodingTablePadChar) {
                    decoded = 0;
                    ++equalsCount;
                }

                *encodedChunkPtr++ = decoded;
            } else {
                *encodedChunkPtr++ = 0;
                ++equalsCount;
            }
        }

        // Treat 8 padding chars as end of input.
        if (equalsCount == 8) {
            break;
        }
        PRIME_ASSERT(equalsCount < 8);

        static const unsigned int equalsCountToMissingBytes[8] = { 0, 1, 1, 2, 3, 3, 4, 4 };

        Base32::DecodeBlock(decodedChunk, encodedChunk);

        decodedChunkSize = 5 - equalsCountToMissingBytes[equalsCount];
        if (destRemaining < decodedChunkSize) {
            return -1;
        }

        destRemaining -= decodedChunkSize;
        copy = decodedChunk;

        switch (decodedChunkSize) {
        case 5:
            *dest++ = *copy++;
        case 4:
            *dest++ = *copy++;
        case 3:
            *dest++ = *copy++;
        case 2:
            *dest++ = *copy++;
        case 1:
            *dest++ = *copy;
        }
    }

    PRIME_ASSERT(dest == (uint8_t*)buffer + bufferSize - destRemaining);

    return dest - (uint8_t*)buffer;
}

size_t Base32ComputeMaxDecodedSize(size_t inputSize)
{
    return (inputSize + 7) / 8 * 5;
}

void Base32BuildDecodingTable()
{
    Base32::BuildDecodingTable();
}

void Base32EncodeAppend(std::string& out, const void* data, size_t dataSize)
{
    size_t maxBase32Size = Base32ComputeMaxEncodedSize(dataSize, 0, 0);

    size_t sizeWas = out.size();

    out.resize(sizeWas + maxBase32Size);

    char* ptr = &out[sizeWas];

    size_t encodedSize = Base32Encode(ptr, maxBase32Size, data, dataSize, 0);
    PRIME_ASSERT(encodedSize <= maxBase32Size);

    out.resize(sizeWas + encodedSize);
}

void Base32EncodeAppend(std::string& out, StringView string)
{
    Base32EncodeAppend(out, string.data(), string.size());
}

bool Base32DecodeAppend(std::string& out, StringView string)
{
    size_t maxDecodedSize = Base32ComputeMaxDecodedSize(string.size());

    size_t sizeWas = out.size();

    out.resize(out.size() + maxDecodedSize + 1);

    ptrdiff_t decodedSize = Base32Decode(&out[0], out.size(), string);

    PRIME_ASSERT(decodedSize != -1 && decodedSize <= (ptrdiff_t)maxDecodedSize);
    if (decodedSize < 0) {
        out.resize(sizeWas);
        return false;
    }

    out.resize(sizeWas + decodedSize);
    return true;
}

std::string Base32Encode(StringView source)
{
    std::string string;
    Base32EncodeAppend(string, source);
    return string;
}

std::string Base32Decode(StringView source)
{
    std::string string;
    Base32DecodeAppend(string, source);
    return string;
}

//
// URL encoding
//

namespace Private {

    /*
            def is_safe(n):
                if n <= 32 or n >= 127:
                    return 0
                if chr(n) in "\",;\\":
                    return 0
                return 1

            for row in range(256/16):
                bits = ["%d" % (is_safe(n)) for n in range(row*16, row*16+16)]
                print "PRIME_BINARY({}), PRIME_BINARY({}),".format("".join(bits[0:8])[::-1], "".join(bits[8:16])[::-1])
        */

    // Safe characters are a..z, A..Z, 0..9 and -_.!~*'{}
    const uint8_t urlSafeLUT[256 / 8] = {
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(10000010),
        PRIME_BINARY(01100100),
        PRIME_BINARY(11111111),
        PRIME_BINARY(00000011),
        PRIME_BINARY(11111110),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(10000111),
        PRIME_BINARY(11111110),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(01101111),
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

    /*
            def is_legal(n):
                if n <= 32 or n >= 127:
                    return False
                if n >= ord('a') and n <= ord('z'):
                    return True
                if n >= ord('A') and n <= ord('Z'):
                    return True
                if n >= ord('0') and n <= ord('9'):
                    return True
                return chr(n) in "-._~:/?#[]@!$&'()*+,;=%"

            for row in range(256/16):
                bits = ["%d" % (is_legal(n)) for n in range(row*16, row*16+16)]
                print "PRIME_BINARY({}), PRIME_BINARY({}),".format("".join(bits[0:8])[::-1], "".join(bits[8:16])[::-1])
        */

    const uint8_t urlLegalLUT[256 / 8] = {
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(00000000),
        PRIME_BINARY(11111010),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(10101111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(10101111),
        PRIME_BINARY(11111110),
        PRIME_BINARY(11111111),
        PRIME_BINARY(11111111),
        PRIME_BINARY(01000111),
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
}

bool IsURLSafe(StringView string)
{
    for (const char *ptr = string.begin(), *end = string.end(); ptr != end; ++ptr) {
        if (!IsURLSafe(*ptr)) {
            return false;
        }
    }

    return true;
}

bool IsURLLegal(StringView string)
{
    for (const char *ptr = string.begin(), *end = string.end(); ptr != end; ++ptr) {
        if (!IsURLLegal(*ptr)) {
            return false;
        }
    }

    return true;
}

size_t URLEncode(char* buffer, size_t bufferSize, StringView source, unsigned int options)
{
    char* dest = buffer;
    char* destEnd = buffer ? buffer + bufferSize : NULL;

    const char* ptr = source.begin();
    const char* end = source.end();

    for (; ptr != end; ++ptr) {
        unsigned char uch = *(const unsigned char*)ptr;
        if (!IsURLSafe(uch)) {
            if (uch == ' ') {
                if (options & URLEncodeFlagSpacesAsPluses) {
                    if (dest < destEnd) {
                        *dest = '+';
                    }
                    ++dest;
                    continue;
                }

                if (options & URLEncodeFlagLeaveSpaces) {
                    if (dest < destEnd) {
                        *dest = ' ';
                    }
                    ++dest;
                    continue;
                }
            }

            static const char hexDigits[] = "0123456789ABCDEF";
            char escaped[4];
            escaped[0] = '%';
            escaped[1] = hexDigits[(uch >> 4) & 0x0f];
            escaped[2] = hexDigits[uch & 0x0f];

            for (const char* escapePtr = escaped; escapePtr != escaped + 3; escapePtr++) {
                if (dest < destEnd) {
                    *dest = *escapePtr;
                }
                ++dest;
            }
        } else {
            if (dest < destEnd) {
                *dest = (char)uch;
            }
            ++dest;
        }
    }

    return (size_t)(dest - buffer);
}

size_t URLDecode(char* buffer, size_t bufferSize, StringView source, unsigned int options)
{
    char* dest = buffer;
    char* destEnd = buffer ? buffer + bufferSize : NULL;

    const char* ptr = source.begin();
    const char* end = source.end();

    while (ptr != end) {
        if (*ptr == '%') {
            if (end - ptr < 3) {
                break; // Discard incomplete escape sequence.
            }

            unsigned int value;
            const char* newPtr;
            if (!ParseHexInt(StringView(ptr + 1, end).substr(0, 2), newPtr, value)) {
                if (dest < destEnd) {
                    *dest = '%';
                }
                ++dest;
                ++ptr;
                continue;
            }

            PRIME_ASSERT(value <= 255);
            if (dest < destEnd) {
                *dest = (char)(unsigned char)value;
            }
            ++dest;
            ptr = newPtr;
        } else if (*ptr == '+' && (options & URLDecodeFlagPlusesAsSpaces)) {
            if (dest < destEnd) {
                *dest = ' ';
            }
            ++dest;
            ++ptr;
        } else {
            if (dest < destEnd) {
                *dest = *ptr;
            }
            ++dest;
            ++ptr;
        }
    }

    return (size_t)(dest - buffer);
}

void URLEncodeAppend(std::string& output, StringView source, unsigned int options)
{
    size_t encodedSize = URLEncode(NULL, 0, source, options);
    if (encodedSize) {
        size_t outputSize = output.size();
        output.resize(outputSize + encodedSize);
        URLEncode(&output[outputSize], encodedSize, source, options);
    }
}

std::string URLEncode(StringView source, unsigned int options)
{
    size_t encodedSize = URLEncode(NULL, 0, source, options);
    std::string string;
    if (encodedSize) {
        string.resize(encodedSize);
        URLEncode(&string[0], encodedSize, source, options);
    }
    return string;
}

void URLDecodeAppend(std::string& output, StringView source, unsigned int options)
{
    size_t decodedSize = URLDecode(NULL, 0, source, options);
    if (decodedSize) {
        size_t outputSize = output.size();
        output.resize(outputSize + decodedSize);
        URLDecode(&output[outputSize], decodedSize, source, options);
    }
}

std::string URLDecode(StringView source, unsigned int options)
{
    size_t decodedSize = URLDecode(NULL, 0, source, options);
    std::string string;
    if (decodedSize) {
        string.resize(decodedSize);
        URLDecode(&string[0], decodedSize, source, options);
    }
    return string;
}

//
// HTML escaping
//

size_t HTMLEscape(char* buffer, size_t bufferSize, StringView source, unsigned int options)
{
    static const char charsToEscape[] = "<>&\r\n";
    static const char charsToEscapeWithQuotes[] = "<>&\"'\r\n";
    static const char* substitutions[] = { "&lt;", "&gt;", "&amp;", "&quot;", "&#39;", "\\0" };

    const char* chars;
    const char** subs;
    if (!(options & HTMLEscapeFlagLeaveQuotes)) {
        chars = charsToEscapeWithQuotes;
        subs = substitutions;
    } else {
        chars = charsToEscape;
        subs = substitutions;
    }

    char* dest = buffer;
    char* destEnd = buffer ? buffer + bufferSize : NULL;

    const char* ptr = source.begin();
    const char* end = source.end();

    for (; ptr != end; ++ptr) {
        if (const char* match = strchr(chars, *ptr)) {
            if (*match == '\r') {
                // Strip \r
            } else if (*match == '\n') {
                if (options & HTMLEscapeFlagNewlinesToBR) {
                    for (const char* br = "<br>"; *br; ++br) {
                        if (dest < destEnd) {
                            *dest = *br;
                        }
                        ++dest;
                    }
                } else {
                    for (const char* crlf = "\r\n"; *crlf; ++crlf) {
                        if (dest < destEnd) {
                            *dest = *crlf;
                        }
                        ++dest;
                    }
                }
            } else {
                for (const char* sub = subs[match - chars]; *sub; ++sub) {
                    if (dest < destEnd) {
                        *dest = *sub;
                    }
                    ++dest;
                }
            }
        } else {
            if (dest < destEnd) {
                *dest = *ptr;
            }
            ++dest;
        }
    }

    return (size_t)(dest - buffer);
}

void HTMLEscapeAppend(std::string& output, StringView source, unsigned int options)
{
    size_t escapedSize = HTMLEscape(NULL, 0, source, options);
    if (escapedSize) {
        size_t outputSize = output.size();
        output.resize(outputSize + escapedSize);
        HTMLEscape(&output[outputSize], escapedSize, source, options);
    }
}

std::string HTMLEscape(StringView source, unsigned int options)
{
    size_t escapedSize = HTMLEscape(NULL, 0, source, options);
    std::string string;
    if (escapedSize) {
        string.resize(escapedSize);
        HTMLEscape(&string[0], escapedSize, source, options);
    }
    return string;
}

//
// HTML unescaping
//

// These are _sometimes_ empty: "object", "style", "script", "textarea", "title"
static const char* htmlEmptyElements[] = {
    // Note these are sorted.
    "area",
    "base",
    "basefont",
    "br",
    "col",
    "command",
    "embed",
    "frame",
    "hr",
    "img",
    "input",
    "isindex",
    "keygen",
    "link",
    "meta",
    "param",
    "source",
    "track",
    "wbr"
};

// http://en.wikipedia.org/wiki/List_of_XML_and_HTML_character_entity_references
static const HTMLEntity htmlEntities[] = {
    { "&quot;", 0x0022, NULL },
    { "&amp;", 0x0026, NULL },
    { "&apos;", 0x0027, NULL },
    { "&lt;", 0x003C, NULL },
    { "&gt;", 0x003E, NULL },
    { "&quot;", 0x0022, NULL },
    { "&amp;", 0x0026, NULL },
    { "&apos;", 0x0027, NULL },
    { "&lt;", 0x003C, NULL },
    { "&gt;", 0x003E, NULL },
    { "&nbsp;", 0x00A0, NULL },
    { "&iexcl;", 0x00A1, NULL },
    { "&cent;", 0x00A2, NULL },
    { "&pound;", 0x00A3, NULL },
    { "&curren;", 0x00A4, NULL },
    { "&yen;", 0x00A5, NULL },
    { "&brvbar;", 0x00A6, NULL },
    { "&sect;", 0x00A7, NULL },
    { "&uml;", 0x00A8, NULL },
    { "&copy;", 0x00A9, NULL },
    { "&ordf;", 0x00AA, NULL },
    { "&laquo;", 0x00AB, NULL },
    { "&not;", 0x00AC, NULL },
    { "&shy;", 0x00AD, NULL },
    { "&reg;", 0x00AE, NULL },
    { "&macr;", 0x00AF, NULL },
    { "&deg;", 0x00B0, NULL },
    { "&plusmn;", 0x00B1, NULL },
    { "&sup2;", 0x00B2, NULL },
    { "&sup3;", 0x00B3, NULL },
    { "&acute;", 0x00B4, NULL },
    { "&micro;", 0x00B5, NULL },
    { "&para;", 0x00B6, NULL },
    { "&middot;", 0x00B7, NULL },
    { "&cedil;", 0x00B8, NULL },
    { "&sup1;", 0x00B9, NULL },
    { "&ordm;", 0x00BA, NULL },
    { "&raquo;", 0x00BB, NULL },
    { "&frac14;", 0x00BC, NULL },
    { "&frac12;", 0x00BD, NULL },
    { "&frac34;", 0x00BE, NULL },
    { "&iquest;", 0x00BF, NULL },
    { "&Agrave;", 0x00C0, NULL },
    { "&Aacute;", 0x00C1, NULL },
    { "&Acirc;", 0x00C2, NULL },
    { "&Atilde;", 0x00C3, NULL },
    { "&Auml;", 0x00C4, NULL },
    { "&Aring;", 0x00C5, NULL },
    { "&AElig;", 0x00C6, NULL },
    { "&Ccedil;", 0x00C7, NULL },
    { "&Egrave;", 0x00C8, NULL },
    { "&Eacute;", 0x00C9, NULL },
    { "&Ecirc;", 0x00CA, NULL },
    { "&Euml;", 0x00CB, NULL },
    { "&Igrave;", 0x00CC, NULL },
    { "&Iacute;", 0x00CD, NULL },
    { "&Icirc;", 0x00CE, NULL },
    { "&Iuml;", 0x00CF, NULL },
    { "&ETH;", 0x00D0, NULL },
    { "&Ntilde;", 0x00D1, NULL },
    { "&Ograve;", 0x00D2, NULL },
    { "&Oacute;", 0x00D3, NULL },
    { "&Ocirc;", 0x00D4, NULL },
    { "&Otilde;", 0x00D5, NULL },
    { "&Ouml;", 0x00D6, NULL },
    { "&times;", 0x00D7, NULL },
    { "&Oslash;", 0x00D8, NULL },
    { "&Ugrave;", 0x00D9, NULL },
    { "&Uacute;", 0x00DA, NULL },
    { "&Ucirc;", 0x00DB, NULL },
    { "&Uuml;", 0x00DC, NULL },
    { "&Yacute;", 0x00DD, NULL },
    { "&THORN;", 0x00DE, NULL },
    { "&szlig;", 0x00DF, NULL },
    { "&agrave;", 0x00E0, NULL },
    { "&aacute;", 0x00E1, NULL },
    { "&acirc;", 0x00E2, NULL },
    { "&atilde;", 0x00E3, NULL },
    { "&auml;", 0x00E4, NULL },
    { "&aring;", 0x00E5, NULL },
    { "&aelig;", 0x00E6, NULL },
    { "&ccedil;", 0x00E7, NULL },
    { "&egrave;", 0x00E8, NULL },
    { "&eacute;", 0x00E9, NULL },
    { "&ecirc;", 0x00EA, NULL },
    { "&euml;", 0x00EB, NULL },
    { "&igrave;", 0x00EC, NULL },
    { "&iacute;", 0x00ED, NULL },
    { "&icirc;", 0x00EE, NULL },
    { "&iuml;", 0x00EF, NULL },
    { "&eth;", 0x00F0, NULL },
    { "&ntilde;", 0x00F1, NULL },
    { "&ograve;", 0x00F2, NULL },
    { "&oacute;", 0x00F3, NULL },
    { "&ocirc;", 0x00F4, NULL },
    { "&otilde;", 0x00F5, NULL },
    { "&ouml;", 0x00F6, NULL },
    { "&divide;", 0x00F7, NULL },
    { "&oslash;", 0x00F8, NULL },
    { "&ugrave;", 0x00F9, NULL },
    { "&uacute;", 0x00FA, NULL },
    { "&ucirc;", 0x00FB, NULL },
    { "&uuml;", 0x00FC, NULL },
    { "&yacute;", 0x00FD, NULL },
    { "&thorn;", 0x00FE, NULL },
    { "&yuml;", 0x00FF, NULL },
    { "&OElig;", 0x0152, NULL },
    { "&oelig;", 0x0153, NULL },
    { "&Scaron;", 0x0160, NULL },
    { "&scaron;", 0x0161, NULL },
    { "&Yuml;", 0x0178, NULL },
    { "&fnof;", 0x0192, NULL },
    { "&circ;", 0x02C6, NULL },
    { "&tilde;", 0x02DC, NULL },
    { "&Alpha;", 0x0391, NULL },
    { "&Beta;", 0x0392, NULL },
    { "&Gamma;", 0x0393, NULL },
    { "&Delta;", 0x0394, NULL },
    { "&Epsilon;", 0x0395, NULL },
    { "&Zeta;", 0x0396, NULL },
    { "&Eta;", 0x0397, NULL },
    { "&Theta;", 0x0398, NULL },
    { "&Iota;", 0x0399, NULL },
    { "&Kappa;", 0x039A, NULL },
    { "&Lambda;", 0x039B, NULL },
    { "&Mu;", 0x039C, NULL },
    { "&Nu;", 0x039D, NULL },
    { "&Xi;", 0x039E, NULL },
    { "&Omicron;", 0x039F, NULL },
    { "&Pi;", 0x03A0, NULL },
    { "&Rho;", 0x03A1, NULL },
    { "&Sigma;", 0x03A3, NULL },
    { "&Tau;", 0x03A4, NULL },
    { "&Upsilon;", 0x03A5, NULL },
    { "&Phi;", 0x03A6, NULL },
    { "&Chi;", 0x03A7, NULL },
    { "&Psi;", 0x03A8, NULL },
    { "&Omega;", 0x03A9, NULL },
    { "&alpha;", 0x03B1, NULL },
    { "&beta;", 0x03B2, NULL },
    { "&gamma;", 0x03B3, NULL },
    { "&delta;", 0x03B4, NULL },
    { "&epsilon;", 0x03B5, NULL },
    { "&zeta;", 0x03B6, NULL },
    { "&eta;", 0x03B7, NULL },
    { "&theta;", 0x03B8, NULL },
    { "&iota;", 0x03B9, NULL },
    { "&kappa;", 0x03BA, NULL },
    { "&lambda;", 0x03BB, NULL },
    { "&mu;", 0x03BC, NULL },
    { "&nu;", 0x03BD, NULL },
    { "&xi;", 0x03BE, NULL },
    { "&omicron;", 0x03BF, NULL },
    { "&pi;", 0x03C0, NULL },
    { "&rho;", 0x03C1, NULL },
    { "&sigmaf;", 0x03C2, NULL },
    { "&sigma;", 0x03C3, NULL },
    { "&tau;", 0x03C4, NULL },
    { "&upsilon;", 0x03C5, NULL },
    { "&phi;", 0x03C6, NULL },
    { "&chi;", 0x03C7, NULL },
    { "&psi;", 0x03C8, NULL },
    { "&omega;", 0x03C9, NULL },
    { "&thetasym;", 0x03D1, NULL },
    { "&upsih;", 0x03D2, NULL },
    { "&piv;", 0x03D6, NULL },
    { "&ensp;", 0x2002, NULL },
    { "&emsp;", 0x2003, NULL },
    { "&thinsp;", 0x2009, NULL },
    { "&zwnj;", 0x200C, NULL },
    { "&zwj;", 0x200D, NULL },
    { "&lrm;", 0x200E, NULL },
    { "&rlm;", 0x200F, NULL },
    { "&ndash;", 0x2013, NULL },
    { "&mdash;", 0x2014, NULL },
    { "&lsquo;", 0x2018, NULL },
    { "&rsquo;", 0x2019, NULL },
    { "&sbquo;", 0x201A, NULL },
    { "&ldquo;", 0x201C, NULL },
    { "&rdquo;", 0x201D, NULL },
    { "&bdquo;", 0x201E, NULL },
    { "&dagger;", 0x2020, NULL },
    { "&Dagger;", 0x2021, NULL },
    { "&bull;", 0x2022, NULL },
    { "&hellip;", 0x2026, NULL },
    { "&permil;", 0x2030, NULL },
    { "&prime;", 0x2032, NULL },
    { "&Prime;", 0x2033, NULL },
    { "&lsaquo;", 0x2039, NULL },
    { "&rsaquo;", 0x203A, NULL },
    { "&oline;", 0x203E, NULL },
    { "&frasl;", 0x2044, NULL },
    { "&euro;", 0x20AC, NULL },
    { "&image;", 0x2111, NULL },
    { "&weierp;", 0x2118, NULL },
    { "&real;", 0x211C, NULL },
    { "&trade;", 0x2122, NULL },
    { "&alefsym;", 0x2135, NULL },
    { "&larr;", 0x2190, NULL },
    { "&uarr;", 0x2191, NULL },
    { "&rarr;", 0x2192, NULL },
    { "&darr;", 0x2193, NULL },
    { "&harr;", 0x2194, NULL },
    { "&crarr;", 0x21B5, NULL },
    { "&lArr;", 0x21D0, NULL },
    { "&uArr;", 0x21D1, NULL },
    { "&rArr;", 0x21D2, NULL },
    { "&dArr;", 0x21D3, NULL },
    { "&hArr;", 0x21D4, NULL },
    { "&forall;", 0x2200, NULL },
    { "&part;", 0x2202, NULL },
    { "&exist;", 0x2203, NULL },
    { "&empty;", 0x2205, NULL },
    { "&nabla;", 0x2207, NULL },
    { "&isin;", 0x2208, NULL },
    { "&notin;", 0x2209, NULL },
    { "&ni;", 0x220B, NULL },
    { "&prod;", 0x220F, NULL },
    { "&sum;", 0x2211, NULL },
    { "&minus;", 0x2212, NULL },
    { "&lowast;", 0x2217, NULL },
    { "&radic;", 0x221A, NULL },
    { "&prop;", 0x221D, NULL },
    { "&infin;", 0x221E, NULL },
    { "&ang;", 0x2220, NULL },
    { "&and;", 0x2227, NULL },
    { "&or;", 0x2228, NULL },
    { "&cap;", 0x2229, NULL },
    { "&cup;", 0x222A, NULL },
    { "&int;", 0x222B, NULL },
    { "&there4;", 0x2234, NULL },
    { "&sim;", 0x223C, NULL },
    { "&cong;", 0x2245, NULL },
    { "&asymp;", 0x2248, NULL },
    { "&ne;", 0x2260, NULL },
    { "&equiv;", 0x2261, NULL },
    { "&le;", 0x2264, NULL },
    { "&ge;", 0x2265, NULL },
    { "&sub;", 0x2282, NULL },
    { "&sup;", 0x2283, NULL },
    { "&nsub;", 0x2284, NULL },
    { "&sube;", 0x2286, NULL },
    { "&supe;", 0x2287, NULL },
    { "&oplus;", 0x2295, NULL },
    { "&otimes;", 0x2297, NULL },
    { "&perp;", 0x22A5, NULL },
    { "&sdot;", 0x22C5, NULL },
    { "&lceil;", 0x2308, NULL },
    { "&rceil;", 0x2309, NULL },
    { "&lfloor;", 0x230A, NULL },
    { "&rfloor;", 0x230B, NULL },
    { "&lang;", 0x2329, NULL },
    { "&rang;", 0x232A, NULL },
    { "&loz;", 0x25CA, NULL },
    { "&spades;", 0x2660, NULL },
    { "&clubs;", 0x2663, NULL },
    { "&hearts;", 0x2665, NULL },
    { "&diams;", 0x2666, NULL },
};

ArrayView<const HTMLEntity> GetHTMLEntities()
{
    return ArrayView<const HTMLEntity>(htmlEntities, PRIME_COUNTOF(htmlEntities));
}

ArrayView<const char*> GetHTMLEmptyElements()
{
    return ArrayView<const char*>(htmlEmptyElements, PRIME_COUNTOF(htmlEmptyElements));
}

size_t HTMLUnescape(char* buffer, size_t bufferSize, StringView source, ArrayView<const HTMLEntity> entities)
{
    char* dest = buffer;
    char* destEnd = buffer ? buffer + bufferSize : NULL;

    const char* ptr = source.begin();
    const char* end = source.end();

    char replacementBuffer[16];

    for (; ptr != end; ++ptr) {
        if (*ptr == '&') {
            const char* semi = reinterpret_cast<const char*>(memchr(ptr + 1, ';', end - ptr - 1));
            if (semi && semi - ptr >= 2) {
                const char* replacement = NULL;

                if (ptr[1] == '#') {
                    if (semi - ptr >= 3 && (ptr[2] == 'x' || ptr[2] == 'X')) {
                        uint32_t n;
                        const char* endPtr;
                        if (ParseHexInt(StringView(ptr + 3, semi), endPtr, n)) {
                            if (endPtr == semi) {
                                replacementBuffer[UTF8Encode(reinterpret_cast<uint8_t*>(replacementBuffer), n)] = '\0';
                                replacement = replacementBuffer;
                            }
                        }
                    } else {
                        uint32_t n;
                        const char* endPtr;
                        if (ParseInt(StringView(ptr + 2, semi), endPtr, n, 10)) {
                            if (endPtr == semi) {
                                replacementBuffer[UTF8Encode(reinterpret_cast<uint8_t*>(replacementBuffer), n)] = '\0';
                                replacement = replacementBuffer;
                            }
                        }
                    }
                } else {
                    for (size_t i = 0; i != entities.size(); ++i) {
                        if (strncmp(entities[i].token, ptr, semi - ptr + 1) == 0) {
                            if (entities[i].string) {
                                replacement = entities[i].string;

                            } else {
                                replacementBuffer[UTF8Encode(reinterpret_cast<uint8_t*>(replacementBuffer), entities[i].entity)] = '\0';
                                replacement = replacementBuffer;
                            }

                            break;
                        }
                    }
                }

                if (replacement) {
                    for (; *replacement; ++replacement, ++dest) {
                        if (dest < destEnd) {
                            *dest = *replacement;
                        }
                    }

                    ptr = semi; // for loop will immediately skip the semicolon
                    continue;
                }
            }

            // Fall through and emit the ampersand as-is
        }

        if (dest < destEnd) {
            *dest = *ptr;
        }
        ++dest;
    }

    return (size_t)(dest - buffer);
}

std::string HTMLUnescape(StringView source)
{
    size_t escapedSize = HTMLUnescape(NULL, 0, source);
    std::string string;
    if (escapedSize) {
        string.resize(escapedSize);
        HTMLUnescape(&string[0], escapedSize, source);
    }
    return string;
}

void HTMLUnescapeAppend(std::string& output, StringView source)
{
    size_t escapedSize = HTMLUnescape(NULL, 0, source);
    if (escapedSize) {
        size_t outputSize = output.size();
        output.resize(outputSize + escapedSize);
        HTMLUnescape(&output[outputSize], escapedSize, source);
    }
}

//
// Encoding bytes as hex
//

bool HexEncode(char* buffer, size_t bufferSize, const void* bytes, size_t bytesSize)
{
    size_t bytesToEncode = Min(bufferSize / 2, bytesSize);
    bool itFit = bytesToEncode == bytesSize;

    bool nullTerminated;
    if (bufferSize > bytesToEncode * 2) {
        buffer[bytesToEncode * 2] = 0;
        nullTerminated = true;
    } else {
        nullTerminated = false;
    }

    const uint8_t* in = (const uint8_t*)bytes;
    const uint8_t* inEnd = in + bytesToEncode;

    char* out = buffer;

    static const char hexDigits[] = "0123456789abcdef";

    for (; in != inEnd; ++in, out += 2) {
        out[0] = hexDigits[(*in) >> 4];
        out[1] = hexDigits[(*in) & 0x0f];
    }

    return nullTerminated && itFit;
}

std::string HexEncode(const void* bytes, size_t bytesSize)
{
    std::string result(bytesSize * 2, 0);

    if (bytesSize) {
        HexEncode(&result[0], result.size(), bytes, bytesSize);
    }

    return result;
}

std::string HexEncode(StringView string)
{
    return HexEncode(string.data(), string.size());
}

namespace {
    template <unsigned int Shift, typename Integer>
    inline bool DecodeHexDigit(char ch, Integer* n)
    {
        // Look-up table?
        if (ch >= '0' && ch <= '9') {
            *n |= (Integer)(ch - '0') << Shift;
        } else if (ch >= 'a' && ch <= 'f') {
            *n |= (Integer)(ch - 'a' + 10) << Shift;
        } else if (ch >= 'A' && ch <= 'F') {
            *n |= (Integer)(ch - 'A' + 10) << Shift;
        } else {
            return false;
        }

        return true;
    }
}

bool HexDecode(void* buffer, size_t bufferSize, StringView digits)
{
    size_t bytesToDecode = Min(digits.size() / 2, bufferSize);
    bool result = digits.size() == bytesToDecode * 2;

    uint8_t* out = (uint8_t*)buffer;
    uint8_t* outEnd = out + bytesToDecode;
    const char* in = digits.data();
    for (; out != outEnd; ++out, in += 2) {
        *out = 0;
        DecodeHexDigit<4>(in[0], out);
        DecodeHexDigit<0>(in[1], out);
    }

    return result;
}

//
// Converting arbitrary text to identifiers
//

std::string EncodeIdentifier(StringView source, const char* safe, char replacement)
{
    std::string result;
    size_t size = source.size();
    result.reserve(size);

    const char* ptr = source.begin();
    const char* end = source.end();

    for (; ptr != end; ++ptr) {
        if (ASCIIIsAlpha(*ptr) || ASCIIIsDigit(*ptr) || strchr(safe, *ptr)) {
            result += *ptr;
        } else if (replacement) {
            result += replacement;
        }
    }

    return result;
}

//
// Email addresses
//

bool IsValidEmailAddress(StringView email)
{
    std::string local, domain;
    return ParseEmailAddress(local, domain, email);
}

bool IsValidEmailAddressStrict(StringView email)
{
    bool strictFail = false;
    std::string local, domain;
    return ParseEmailAddress(local, domain, email, &strictFail) && !strictFail;
}

bool ParseEmailAddress(std::string& localPart, std::string& domain, StringView email, bool* strictFail)
{
    bool strictFailDummy;
    if (!strictFail) {
        strictFail = &strictFailDummy;
    }

    *strictFail = false;

    const char* start = email.begin();
    const char* ptr = email.begin();
    const char* end = email.end();

    for (; ptr != end; ++ptr) {
        if (*ptr >= 'a' && *ptr <= 'z') {
            continue;
        }
        if (*ptr >= 'A' && *ptr <= 'Z') {
            continue;
        }
        if (*ptr >= '0' && *ptr <= '9') {
            continue;
        }

        if (*ptr == '.') {
            if (ptr == start) {
                *strictFail = true;
            } else if (ptr[-1] == '.') {
                *strictFail = true;
            } else if (ptr + 1 != end && ptr[1] == '@') {
                *strictFail = true;
            }

            continue;
        }

        if (strchr("!#$%&'*+-/=?^_`{}|~", *ptr)) {
            continue;
        }

        if (*ptr == '"') {
            ++ptr;
            for (;;) {
                while (ptr != end && *ptr != '"' && *ptr != '\\') {
                    ++ptr;
                }

                if (ptr != end && *ptr == '\\') {
                    ++ptr;
                    if (ptr != end) {
                        ++ptr;
                    }
                    continue;
                }

                break;
            }

            if (ptr == end) {
                break;
            }

            continue;
        }

        break;
    }

    if (ptr == end || *ptr != '@' || ptr == start) {
        return false;
    }

    localPart.assign(start, ptr);
    ++ptr;

    start = ptr;
    for (;;) {
        const char* labelStart = ptr;
        for (; ptr != end; ++ptr) {
            if (*ptr >= 'a' && *ptr <= 'z') {
                continue;
            }
            if (*ptr >= 'A' && *ptr <= 'Z') {
                continue;
            }
            if (*ptr >= '0' && *ptr <= '9') {
                continue;
            }

            if (ptr == labelStart) {
                // First character of the label must be [A-Za-z0-9]
                break;
            }

            if (*ptr == '-') {
                continue;
            }

            break;
        }

        if (ptr == labelStart) {
            // Label can't be empty.
            return false;
        }

        if (ptr != end && *ptr != '.') {
            return false;
        }

        if (ptr[-1] == '-') {
            *strictFail = true; // A label can't end with a hyphen
        }

        if (ptr == end) {
            break;
        }

        ++ptr;
        continue;
    }

    domain.assign(start, ptr);
    return true;
}

//
// MIME filenames
//

std::string MIMEFilenameEncode(StringView input)
{
    static const char validChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._-+,@$!~'=()[]{}0123456789";

    std::string output;
    output.reserve(input.size());

    for (auto ch : input) {
        if (strchr(validChars, ch)) {
            output += ch;
        } else {
            output += '_';
        }
    }

    return output;
}
}
