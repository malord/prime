// Copyright 2000-2021 Mark H. P. Lord

#include "DeflateStream.h"

#ifdef PRIME_HAVE_DEFLATESTREAM

namespace Prime {

DeflateStream::DeflateStream()
{
    _level = 9;
    _begun = false;
    _bufferSize = 0;
    _buffer = NULL;
}

DeflateStream::~DeflateStream()
{
    end(Log::getNullLog());
}

void DeflateStream::setBuffer(size_t size, char* buffer)
{
    PRIME_ASSERT(!_begun);

    if (buffer) {
        _buffer = buffer;
        _deleteBuffer.reset();
    } else {
        _deleteBuffer.reset(new char[size]);
        _buffer = _deleteBuffer.get();
    }

    _bufferSize = size;
}

bool DeflateStream::init(Stream* destination, Log* log, size_t bufferSize, char* buffer)
{
    setBuffer(bufferSize, buffer);

    // You should call end() before starting another deflate.
    PRIME_ASSERT(!_begun);

    // Need a buffer
    PRIME_ASSERT(_buffer);

    _bufferPtr = _buffer;
    _bufferTop = _buffer + _bufferSize;

    _dest = destination;

    _zstream.zalloc = (alloc_func)0;
    _zstream.zfree = (free_func)0;
    _zstream.opaque = (voidpf)0;

    _zstream.next_in = 0;
    _zstream.avail_in = 0;

    _zstream.next_out = (Bytef*)_bufferPtr;
    _zstream.avail_out = (uInt)_bufferSize;
    _zstream.total_in = 0;
    _zstream.total_out = 0;

    // Use of -MAX_WBITS is an undocumented feature of zlib, but it's used in one of their examples. It prevent
    // gzip headers being written.
    int err = deflateInit2(&_zstream, _level, Z_DEFLATED, -MAX_WBITS, _level, 0);

    if (err != Z_OK) {
        logZlibError(log, err);
        cleanup();
        return false;
    }

    _begun = true;

    return true;
}

void DeflateStream::cleanup()
{
    _begun = false;
}

void DeflateStream::abort()
{
    deflateEnd(&_zstream);
    cleanup();
}

bool DeflateStream::flushBuffer(Log* log)
{
    PRIME_ASSERT(_begun);

    if (!_dest->writeExact(_buffer, _bufferPtr - _buffer, log)) {
        abort();
        return false;
    }

    _bufferPtr = _buffer;
    return true;
}

bool DeflateStream::end(Log* log)
{
    // You can call end() without first calling begin().
    if (!_begun) {
        return true;
    }

    // Finish off writing compressed data.
    int err;
    ptrdiff_t space;

    for (;;) {
        // Work out how much space we have in the output buffer
        space = _bufferTop - _bufferPtr;

        // If no space in output buffer, flush the buffer
        if (!space) {
            if (!flushBuffer(log)) {
                return false;
            }

            space = _bufferSize;
        }

        // Prepare output pointers
        _zstream.next_out = (Bytef*)_bufferPtr;
        _zstream.avail_out = (uInt)space;

        // Prepare input pointers
        _zstream.next_in = 0;
        _zstream.avail_in = 0;

        // Deflate
        err = deflate(&_zstream, Z_FINISH);
        _bufferPtr = (char*)_zstream.next_out;
        if (err != Z_OK) {
            break;
        }
    }

    deflateEnd(&_zstream);

    // Flush the remaining output
    if (_bufferPtr != _buffer) {
        if (!_dest->writeExact(_buffer, _bufferPtr - _buffer, log)) {
            cleanup();
            return false;
        }
    }

    cleanup();

    if (err != Z_OK && err != Z_STREAM_END) {
        logZlibError(log, err);
        return false;
    }

    return true;
}

ptrdiff_t DeflateStream::writeSome(const void* bytes, size_t byteCount, Log* log)
{
    PRIME_ASSERT(_begun);

    if (!byteCount) {
        return 0;
    }

    // Work out how much space we have in the output buffer
    ptrdiff_t space = _bufferTop - _bufferPtr;

    // If no space in output buffer, flush the buffer
    if (!space) {
        if (!flushBuffer(log)) {
            return -1;
        }

        space = _bufferSize;
    }

    // Prepare output pointers
    _zstream.next_out = (Bytef*)_bufferPtr;
    _zstream.avail_out = (uInt)space;

    // Prepare input pointers
    _zstream.next_in = (Bytef*)bytes;
    _zstream.avail_in = (uInt)(size_t)byteCount;

    // Deflate
    int err = deflate(&_zstream, Z_NO_FLUSH);
    if (err != Z_OK && err != Z_STREAM_END) {
        logZlibError(log, err);
        abort();
        return -1;
    }

    // Update our buffer pointers
    _bufferPtr = (char*)_zstream.next_out;

    // Figure out how much was read and update the total
    ptrdiff_t bytesWritten = _zstream.next_in - (Bytef*)bytes;

    if (bytesWritten == 0) {
        return writeSome(bytes, byteCount, log);
    }

    return bytesWritten;
}

void DeflateStream::logZlibError(Log* log, int err)
{
    log->error(PRIME_LOCALISE("zlib error %d."), err);
}

bool DeflateStream::close(Log* log)
{
    bool success = end(log);

    if (_dest.get()) {
        if (!_dest->close(log)) {
            success = false;
        }

        _dest.release();
    }

    return success;
}
}

#endif // PRIME_HAVE_DEFLATESTREAM
