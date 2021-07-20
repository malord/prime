// Copyright 2000-2021 Mark H. P. Lord

#include "InflateStream.h"

#ifdef PRIME_HAVE_INFLATESTREAM

namespace Prime {

InflateStream::InflateStream()
{
    _begun = false;
    _eof = false;
    _sizeKnown = -1;
    _bufferSize = 0;
    _buffer = NULL;
}

InflateStream::~InflateStream()
{
    end();
}

void InflateStream::setBuffer(size_t size, char* buffer)
{
    PRIME_ASSERT(!_begun);

    if (!buffer) {
        _deleteBuffer.reset(new char[size]);
        _buffer = _deleteBuffer.get();
    } else {
        _deleteBuffer.reset();
        _buffer = buffer;
    }

    _bufferSize = size;
}

bool InflateStream::init(Stream* sourceStream, Log* log, size_t bufferSize, char* buffer)
{
    setBuffer(bufferSize, buffer);

    // You should call end() before starting another inflate.
    PRIME_ASSERT(!_begun);

    // Need a buffer.
    PRIME_ASSERT(_bufferSize);

    _bufferPtr = _bufferTop = _buffer;

    _source = sourceStream;

    _zstream.zalloc = (alloc_func)0;
    _zstream.zfree = (free_func)0;
    _zstream.opaque = (voidpf)0;

    // These need to be zerod to stop inflateInit reading a header.
    _zstream.next_in = 0;
    _zstream.avail_in = 0;

    // -MAX_WBITS causes inflate to not try and read a gzip header.. this is undocumented but is used in one of
    // their examples.
    int err = inflateInit2(&_zstream, -MAX_WBITS);

    if (err != Z_OK) {
        logZlibError(log, err);
        return false;
    }

    _begun = true;
    _eof = false;

    return true;
}

void InflateStream::cleanup()
{
    // _source.release();
    _begun = false;
    _eof = false;
}

void InflateStream::end()
{
    // You can call end() without first calling begin().
    if (_begun) {
        inflateEnd(&_zstream);
        cleanup();
    }
}

bool InflateStream::close(Log* log)
{
    end();

    bool success;
    if (!_source) {
        success = true;
    } else {
        success = _source->close(log);
        _source.release();
    }

    return success;
}

ptrdiff_t InflateStream::readSome(void* memory, size_t maximumBytes, Log* log)
{
    PRIME_ASSERT(_begun || _eof);

    if (!maximumBytes || _eof) {
        return 0;
    }

    ptrdiff_t bytesRead = 0;

    ptrdiff_t remaining = maximumBytes;
    uint8_t* outPtr = (uint8_t*)memory;

    // Loop until we've read something, or found the end of the file
    while (!bytesRead) {
        // Prepare output pointers
        _zstream.next_out = outPtr;
        _zstream.avail_out = (uInt)remaining;

        // Nothing in the buffer?
        if (_bufferPtr == _bufferTop) {
            // Fill the buffer
            ptrdiff_t readResult = _source->readSome(_buffer, _bufferSize, log);

            if (readResult < 0) {
                end();
                return -1;
            }

            ptrdiff_t numberReadFromSubStream = readResult;

            _bufferPtr = _buffer;
            _bufferTop = _bufferPtr + numberReadFromSubStream;

            if (numberReadFromSubStream == 0) {
                *_bufferPtr = 0;
            }
        }

        // Prepare the input buffer pointers
        _zstream.next_in = (Bytef*)_bufferPtr;
        _zstream.avail_in = (uInt)(size_t)(_bufferTop - _bufferPtr);

        // Inflate as much as possible
        int err = inflate(&_zstream, Z_NO_FLUSH);
        if (err == Z_BUF_ERROR && _bufferTop == _bufferPtr) {
            // This is an odd case. This occurs in Z_BUF_ERROR.zip (a file I made that demonstrates the problem).
            err = Z_STREAM_END;
        }

        if (err != Z_OK && err != Z_STREAM_END) {
            logZlibError(log, err);
            end();
            return -1;
        }

        // Update our buffer pointers
        _bufferPtr = (char*)_zstream.next_in;

        // Figure out how much we got, and update the total
        ptrdiff_t numberOfBytesWeGot = _zstream.next_out - outPtr;
        bytesRead += numberOfBytesWeGot;

        // Shift the memory pointers
        outPtr = _zstream.next_out;
        remaining -= numberOfBytesWeGot;

        // Check for end of stream
        if (err == Z_STREAM_END) {
            end();
            _eof = true;
            break;
        }
    }

    return bytesRead;
}

int64_t InflateStream::getSize(Log*)
{
    return _sizeKnown;
}

void InflateStream::logZlibError(Log* log, int err)
{
    log->error(PRIME_LOCALISE("zlib error %d."), err);
}
}

#endif // PRIME_HAVE_INFLATESTREAM
