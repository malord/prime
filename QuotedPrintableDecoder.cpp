// Copyright 2000-2021 Mark H. P. Lord

#include "QuotedPrintableDecoder.h"
#include "Convert.h"
#include "StringUtils.h"

namespace Prime {

QuotedPrintableDecoder::QuotedPrintableDecoder()
{
    construct();
}

QuotedPrintableDecoder::QuotedPrintableDecoder(StreamBuffer* buffer)
{
    construct();
    begin(buffer);
}

QuotedPrintableDecoder::~QuotedPrintableDecoder()
{
}

void QuotedPrintableDecoder::construct()
{
    _started = false;
}

void QuotedPrintableDecoder::begin(StreamBuffer* buffer)
{
    _buffer = buffer;

    _started = true;
}

ptrdiff_t QuotedPrintableDecoder::readSome(void* memory, size_t maximumBytes, Log* log)
{
    PRIME_ASSERT(_started);

    uint8_t* destPtr = (uint8_t*)memory;
    uint8_t* destEnd = destPtr + maximumBytes;

    const char* ptr = _buffer->getReadPointer();
    const char* top = _buffer->getTopPointer();

    for (;;) {
        if (ptr == top) {
            ptrdiff_t got = _buffer->fetchMore(log);
            if (got < 0) {
                return -1;
            }

            if (got == 0) {
                break;
            }

            ptr = _buffer->getReadPointer();
            top = _buffer->getTopPointer();
        }

        while (ptr != top && destPtr != destEnd && *ptr != '=') {
            *destPtr++ = (uint8_t)*ptr++;
        }

        _buffer->setReadPointer(ptr);

        if (destPtr == destEnd) {
            break;
        }

        if (ptr == top) {
            continue;
        }

        PRIME_ASSERT(*ptr == '=');
        _buffer->skipByte(); // Consume the =

        ptrdiff_t got = _buffer->requestNumberOfBytes(2, log);
        if (got < 0) {
            return -1;
        }

        ptr = _buffer->getReadPointer();
        top = _buffer->getTopPointer();

        if (got == 0 || (ASCIIIsHexDigit(*ptr) && got < 2)) {
            *destPtr++ = '=';
            continue;
        }

        if (ASCIIIsHexDigit(*ptr)) {
            PRIME_DEBUG_ASSERT(got >= 2);

            unsigned int n;
            const char* newPtr;
            if (!ParseHexInt(StringView(ptr, top).substr(0, 2), newPtr, n) || newPtr != ptr + 2) {
                // Not hex!
                *destPtr++ = '=';
                continue;
            }

            *destPtr++ = (uint8_t)n;
            ptr = newPtr;

        } else if (*ptr == '\r') {
            ++ptr;
            if (got >= 2 && *ptr == '\n') {
                ++ptr;
            }

        } else if (*ptr == '\n') {
            ++ptr;
            if (got >= 2 && *ptr == '\r') {
                ++ptr;
            }

        } else {
            // Rogue =
            *destPtr++ = '=';
        }

        _buffer->setReadPointer(ptr);
    }

    return destPtr - (uint8_t*)memory;
}

bool QuotedPrintableDecoder::close(Log* log)
{
    if (_started) {
        _started = false;
        return _buffer->close(log);
    }

    return true;
}

bool QuotedPrintableDecoder::flush(Log* log)
{
    if (_started) {
        return _buffer->flush(log);
    }

    return true;
}
}
