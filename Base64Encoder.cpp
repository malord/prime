// Copyright 2000-2021 Mark H. P. Lord

#include "Base64Encoder.h"
#include "TextEncoding.h"

namespace Prime {

Base64Encoder::Base64Encoder()
{
    construct();
}

Base64Encoder::Base64Encoder(Stream* stream, const Options& options)
{
    construct();
    begin(stream, options);
}

Base64Encoder::~Base64Encoder()
{
    end(Log::getNullLog());
}

void Base64Encoder::construct()
{
    _started = false;
    _bufferSize = 0;
    _maxLineLength = 0;
}

void Base64Encoder::begin(Stream* stream, const Options& options)
{
    _stream = stream;
    _options = options;

    _maxLineLength = _options.getLineLength();
    _bufferSize = std::max<size_t>(_maxLineLength, 128);
    _buffer.reset(new uint8_t[_bufferSize + 2]);
    _bufferLength = 0;

    _blockLength = 0;

    _started = true;
}

ptrdiff_t Base64Encoder::writeSome(const void* memory, size_t maximumBytes, Log* log)
{
    PRIME_ASSERT(_started);

    if (maximumBytes == 0) {
        return 0;
    }

    const uint8_t* ptr = (const uint8_t*)memory;
    const uint8_t* end = ptr + maximumBytes;

    uint8_t block[3] = { _block[0], _block[1], _block[2] };
    unsigned int blockLength = _blockLength;

    if (blockLength != 0) {
        for (; ptr != end && blockLength != 3; ++ptr) {
            block[blockLength++] = *ptr;
        }
    }

    for (;;) {
        PRIME_DEBUG_ASSERT(blockLength == 3 || blockLength == 0);

        if (blockLength == 3) {
            if (_bufferLength + 4 > _bufferSize) {
                if (!flushBuffer(log)) {
                    return -1;
                }
            }

            Base64::EncodeBlock(&_buffer[_bufferLength], block);
            _bufferLength += 4;
        }

        if (end - ptr < 3) {
            // Next call to writeSome gets these.
            _blockLength = 0;
            while (ptr != end) {
                _block[_blockLength++] = *ptr++;
            }
            break;
        }

        block[0] = *ptr++;
        block[1] = *ptr++;
        block[2] = *ptr++;
        blockLength = 3;
    }

    return ptr - (const uint8_t*)memory;
}

bool Base64Encoder::flushBuffer(Log* log, bool atEnd)
{
    PRIME_ASSERT(_bufferLength <= _bufferSize);

    if (_bufferLength <= _maxLineLength || _maxLineLength == 0) {
        if (!atEnd && _maxLineLength != 0) {
            _buffer[_bufferLength++] = '\r';
            _buffer[_bufferLength++] = '\n';
        }

        if (!_stream->writeExact(_buffer.get(), _bufferLength, log)) {
            return false;
        }

        _bufferLength = 0;
        return true;
    }

    size_t remaining = _bufferLength;
    unsigned char* ptr = _buffer.get();
    unsigned char saved0 = 0, saved1 = 0;

    while (remaining) {
        size_t thisLine = std::min<size_t>(remaining, _maxLineLength);
        if (thisLine != _maxLineLength) {
            if (!atEnd) {
                // Keep the rest of the line until we build a full line.
                memmove(_buffer.get(), ptr, remaining);
                _bufferLength = remaining;
                return true;
            }
        }

        size_t thisLineWithCRLF = thisLine;

        if (!atEnd || remaining > thisLine) {
            saved0 = ptr[thisLine];
            saved1 = ptr[thisLine + 1];
            ptr[thisLine] = '\r';
            ptr[thisLine + 1] = '\n';
            thisLineWithCRLF += 2;
        }

        bool ok = _stream->writeExact(ptr, thisLineWithCRLF, log);

        if (thisLineWithCRLF != thisLine) {
            ptr[thisLine] = saved0;
            ptr[thisLine + 1] = saved1;
        }

        ptr += thisLine;
        remaining -= thisLine;
        if (!ok) {
            return false;
        }
    }

    _bufferLength = 0;
    return true;
}

bool Base64Encoder::end(Log* log)
{
    if (!_started) {
        return true;
    }

    _started = false;

    if (_blockLength) {
        if (_bufferLength + 4 > _bufferSize) {
            if (!flushBuffer(log)) {
                return false;
            }
        }

        for (unsigned int i = _blockLength; i != 3; ++i) {
            _block[i] = 0;
        }

        Base64::EncodeBlock(&_buffer[_bufferLength], _block, _blockLength);
        _bufferLength += 4;

        _blockLength = 0;
    }

    if (_bufferLength) {
        if (!flushBuffer(log, true)) {
            return false;
        }
    }

    return true;
}

bool Base64Encoder::close(Log* log)
{
    if (_started) {
        bool ok = end(log);
        _started = false;
        return _stream->close(log) && ok;
    }

    return true;
}

bool Base64Encoder::flush(Log* log)
{
    if (_started) {
        return _stream->flush(log);
    }

    return true;
}
}
