// Copyright 2000-2021 Mark H. P. Lord

#include "StreamBuffer.h"
#include "NumberUtils.h"
#include <string.h>

namespace Prime {

PRIME_DEFINE_UID_CAST(StreamBuffer)

StreamBuffer::StreamBuffer()
{
    zero();
}

void StreamBuffer::zero()
{
    _underlyingOffset = 0;
    _buffer = NULL;

    _top = NULL;
    _ptr = NULL;
    _end = NULL;
    _dirtyBegin = NULL;
    _dirtyEnd = NULL;
    _bufferOffset = 0;
    _seekable = false;

    _maxPutBack = 0;
    _error = false;
}

StreamBuffer::StreamBuffer(Stream* underlyingStream, size_t bufferSize, void* buffer)
{
    zero();
    init(underlyingStream, bufferSize, buffer);
}

StreamBuffer::StreamBuffer(const void* bytes, size_t byteCount)
{
    zero();
    init(bytes, byteCount);
}

StreamBuffer::~StreamBuffer()
{
    unbuffer(false, Log::getGlobal());
}

bool StreamBuffer::init(Stream* underlyingStream, size_t bufferSize, void* buffer)
{
    PRIME_ASSERT(isEmpty());

    if (!PRIME_GUARD(underlyingStream)) {
        return false;
    }

    if (!underlyingStream->isSeekable()) {
        _seekable = false;
        _bufferOffset = 0;
    } else {
        _bufferOffset = underlyingStream->getOffset(Log::getNullLog());
        if (_bufferOffset < 0) {
            return false;
        }

        _seekable = true;
    }

    _underlyingStream = underlyingStream;
    _underlyingOffset = _bufferOffset;

    PRIME_ASSERT(bufferSize != 0);

    if (!buffer) {
        _allocatedBuffer.reset(new char[bufferSize]);
        _buffer = _allocatedBuffer.get();
    } else {
        _allocatedBuffer.reset();
        _buffer = (char*)buffer;
    }

    _top = _ptr = _buffer;
    _end = _buffer + bufferSize;
    _dirtyBegin = _end;
    _dirtyEnd = _buffer;
    _error = false;
    _const = false;

    return true;
}

void StreamBuffer::init(const void* bytes, size_t byteCount)
{
    PRIME_ASSERT(isEmpty());

    _allocatedBuffer.reset();
    _underlyingStream.release();
    _underlyingOffset = _bufferOffset = 0;
    _seekable = false;
    _buffer = (char*)bytes;
    _ptr = _buffer;
    _top = _end = _buffer + byteCount;
    _dirtyBegin = _end;
    _dirtyEnd = _buffer;
    _error = false;
    _const = true;
}

bool StreamBuffer::close(Log* log)
{
    bool success = unbuffer(false, log);

    if (_underlyingStream && !_underlyingStream->close(log)) {
        success = false;
        _error = true;
        _underlyingStream.release();
    }

    return success && !_error;
}

ptrdiff_t StreamBuffer::readSome(void* buffer, size_t maxBytes, Log* log)
{
    for (;;) {
        size_t available = (size_t)(_top - _ptr);

        // If we have bytes available in the buffer, read them and return. We'll re-fill the buffer on the next call.
        if (available) {
            size_t take = Min(available, maxBytes);

            memcpy(buffer, _ptr, take);

            _ptr += take;

            return take;
        }

        // TODO: if reading more than the size of the buffer, have _underlyingStream read directly in to buffer.

        ptrdiff_t got = fetchMore(log);

        if (got < 0) {
            return got;
        }

        if (got == 0) {
            return 0;
        }
    }
}

ptrdiff_t StreamBuffer::fetchMore(Log* log)
{
    if (!_underlyingStream) {
        return 0;
    }

    if (!shift(log)) {
        return -1;
    }

    // Seek to where we should be.
    Offset readOffset = _bufferOffset + (_top - _buffer);
    if (readOffset != _underlyingOffset) {
        PRIME_ASSERTMSG(_seekable, "StreamBuffer had to seek");

        if (!_underlyingStream->setOffset(readOffset, log)) {
            _error = true;
            return -1;
        }

        _underlyingOffset = readOffset;
    }

    // Now try to fill the new space with content from the
    size_t space = (size_t)(_end - _top);

    ptrdiff_t got = _underlyingStream->readSome(_top, space, log);
    if (got < 0) {
        _error = true;
        return -1;
    }

    if (got == 0) {
        return 0;
    }

    _underlyingOffset += got;
    _top += got;
    return got;
}

ptrdiff_t StreamBuffer::writeSome(const void* bytes, size_t maxBytes, Log* log)
{
    PRIME_ASSERT(!isReadOnly());

    for (;;) {
        size_t space = getSpace();

        // If we have space in the buffer, use it and return.
        if (space) {
            size_t take = Min(space, maxBytes);

            memcpy(_ptr, bytes, take);
            advanceWritePointer(take);
            return take;
        }

        // Otherwise, discard everything before _ptr and shift the buffer along to make more room.
        if (!shift(log)) {
            return -1;
        }

        // If we have no room, the underlying Stream's writeSome returned zero.
        if (_ptr == _end) {
            return 0;
        }
    }
}

bool StreamBuffer::shift(Log* log)
{
    if (!flushWrites(log)) {
        return false;
    }

    PRIME_ASSERT(_dirtyBegin == _end && _dirtyEnd == _buffer);

    if (_ptr > _buffer + _maxPutBack) {
        // _ptr is not at the start of the buffer, so we can shift the bytes down to make room at the end of the
        // buffer. We need to take _maxPutBack in to account though.

        size_t inBuffer = getBytesAvailable() + _maxPutBack;

        _bufferOffset += (_ptr - _maxPutBack) - _buffer;
        memmove(_buffer, _ptr - _maxPutBack, inBuffer);
        _top = (char*)_buffer + inBuffer;
        _ptr = (char*)_buffer + _maxPutBack;
    }

    return true;
}

Stream::Offset StreamBuffer::seek(Offset offset, SeekMode mode, Log* log)
{
    Offset currentOffset = _bufferOffset + (_ptr - _buffer);
    Offset newOffset;
    switch (mode) {
    default:
    case SeekModeAbsolute:
        newOffset = offset;
        break;

    case SeekModeRelative:
        newOffset = currentOffset + offset;
        break;

    case SeekModeRelativeToEnd: {
        Offset end = getSize(log);
        if (end < 0) {
            return -1;
        }

        newOffset = end + offset;
        break;
    }
    }

    if (newOffset >= _bufferOffset && newOffset < _bufferOffset + (_top - _buffer)) {
        // Seeking within our buffer!
        _ptr = _buffer + (size_t)(newOffset - _bufferOffset);
        return newOffset;
    }

    PRIME_ASSERT(_underlyingStream); // Should have been a seek within our buffer.

    if (!unbuffer(false, log)) {
        return -1;
    }

    PRIME_ASSERTMSG(_seekable, "StreamBuffer had to seek");
    _underlyingOffset = _underlyingStream->seek(newOffset, SeekModeAbsolute, log);
    if (_underlyingOffset < 0) {
        _error = true;
    }
    _bufferOffset = _underlyingOffset;
    return _underlyingOffset;
}

Stream::Offset StreamBuffer::getSize(Log* log)
{
    if (!_underlyingStream) {
        return getBufferSize();
    }

    // This probably isn't necessary - we can work out the file offset we'll have after the write
    if (!flushWrites(log)) {
        return -1;
    }

    return _underlyingStream->getSize(log);
}

bool StreamBuffer::setSize(Offset newSize, Log* log)
{
    if (!unbuffer(false, log)) {
        return false;
    }

    if (!_underlyingStream->setSize(newSize, log)) {
        _error = true;
    }

    _underlyingOffset = _underlyingStream->getOffset(Log::getNullLog());
    if (_underlyingOffset < 0) {
        _error = true;
    }

    return !_error;
}

bool StreamBuffer::flush(Log* log)
{
    if (!shift(log)) {
        return false;
    }

    if (_underlyingStream && !_underlyingStream->flush(log)) {
        _error = true;
    }

    return !_error;
}

bool StreamBuffer::flushWrites(Log* log)
{
    if (_dirtyBegin >= _dirtyEnd) {
        return true;
    }

    PRIME_ASSERT(!isReadOnly());

    Offset dirtyOffset = _bufferOffset + (_dirtyBegin - _buffer);
    if (_underlyingOffset != dirtyOffset) {
        PRIME_ASSERTMSG(_seekable, "StreamBuffer had to seek");

        if (!_underlyingStream->setOffset(dirtyOffset, log)) {
            _error = true;
            return false;
        }

        _underlyingOffset = dirtyOffset;
    }

    size_t byteCount = (size_t)(_dirtyEnd - _dirtyBegin);

    if (!_underlyingStream->writeExact(_dirtyBegin, byteCount, log)) {
        _error = true;
        return false;
    }

    _underlyingOffset += byteCount;

    _dirtyBegin = _end;
    _dirtyEnd = _buffer;

    return true;
}

bool StreamBuffer::unbuffer(bool seekBack, Log* log)
{
    bool success = flushWrites(log);

    Stream::Offset currentOffset = _bufferOffset + (_ptr - _buffer);

    if (currentOffset != _underlyingOffset) {
        if (seekBack) {
            PRIME_ASSERT(_seekable);

            if (!_underlyingStream->setOffset(currentOffset, log)) {
                _error = true;
                success = false;
            }

            _underlyingOffset = currentOffset;
        }
    }

    _ptr = _top = _buffer;
    _bufferOffset = _underlyingOffset;

    return success;
}

bool StreamBuffer::writeByte(int c, Log* log)
{
    PRIME_DEBUG_ASSERT(!isReadOnly());

    if (_ptr == _end) {
        return writeByteBufferFull(c, log);
    }

    if (_ptr < _dirtyBegin) {
        _dirtyBegin = _ptr;
    }

    *_ptr++ = (char)(uint8_t)(unsigned int)c;

    if (_ptr > _dirtyEnd) {
        _dirtyEnd = _ptr;
    }
    if (_ptr > _top) {
        _top = _ptr;
    }

    return true;
}

bool StreamBuffer::writeBytes(const void* memory, size_t size, Log* log)
{
    while (size) {
        if (!getSpace()) {
            if (!unbuffer(false, log)) {
                return false;
            }
        }

        size_t copy = size < getSpace() ? size : getSpace();
        memcpy(_ptr, memory, copy);

        advanceWritePointer(copy);

        size -= copy;
        memory = (const char*)memory + copy;
    }

    return true;
}

bool StreamBuffer::writeByteBufferFull(int c, Log* log)
{
    if (!unbuffer(false, log)) {
        return false;
    }

    return writeByte(c, log);
}

void* StreamBuffer::makeSpaceBufferFull(size_t needed, Log* log)
{
    PRIME_ASSERT(getBufferSize());

    if (!unbuffer(false, log)) {
        return NULL;
    }

    PRIME_ASSERT(needed <= getBufferSize());
    if (getSpace() < needed) {
        return NULL;
    }

    return _ptr;
}

const char* StreamBuffer::requireNumberOfBytesRefillBuffer(size_t byteCount, Log* log)
{
    while (getBytesAvailable() < byteCount) {
        if (fetchMore(log) <= 0) {
            return NULL;
        }
    }

    return getReadPointer();
}

int StreamBuffer::readByteRefillBuffer(Log* log)
{
    if (fetchMore(log) <= 0) {
        return -1;
    }

    return (int)(uint8_t)(*_ptr++);
}

bool StreamBuffer::readByteRefillBuffer(uint8_t& ch, Log* log)
{
    if (fetchMore(log) <= 0) {
        return false;
    }

    ch = *_ptr++;
    return true;
}

int StreamBuffer::peekByteRefillBuffer(size_t offset, Log* log)
{
    if (!requireNumberOfBytes(offset + 1, log)) {
        return -1;
    }

    return (int)(uint8_t)(_ptr[offset]);
}

bool StreamBuffer::peekByteRefillBuffer(size_t offset, uint8_t& ch, Log* log)
{
    if (!requireNumberOfBytes(offset + 1, log)) {
        return false;
    }

    ch = (_ptr[offset]);
    return true;
}

bool StreamBuffer::peekBytes(void* memory, size_t byteCount, Log* log)
{
    PRIME_ASSERT(byteCount <= getBufferSize());

    if (!requireNumberOfBytes(byteCount, log)) {
        return false;
    }

    memcpy(memory, _ptr, byteCount);
    return true;
}

bool StreamBuffer::peekBytes(size_t offset, void* memory, size_t byteCount, Log* log)
{
    PRIME_ASSERT(byteCount + offset <= getBufferSize());

    if (!requireNumberOfBytes(byteCount + offset, log)) {
        return false;
    }

    memcpy(memory, _ptr + offset, byteCount);
    return true;
}

bool StreamBuffer::matchBytes(const void* bytes, size_t byteCount, Log* log)
{
    PRIME_ASSERT(byteCount <= getBufferSize());

    if (!requireNumberOfBytes(byteCount, log)) {
        return false;
    }

    return memcmp(_ptr, bytes, byteCount) == 0;
}

bool StreamBuffer::matchBytes(size_t offset, const void* bytes, size_t byteCount, Log* log)
{
    PRIME_ASSERT(byteCount + offset <= getBufferSize());

    if (!requireNumberOfBytes(byteCount + offset, log)) {
        return false;
    }

    return memcmp(_ptr + offset, bytes, byteCount) == 0;
}

bool StreamBuffer::skipMatchingBytes(const void* bytes, size_t byteCount, Log* log)
{
    if (!matchBytes(bytes, byteCount, log)) {
        return false;
    }

    advanceReadPointer(byteCount);
    return true;
}

bool StreamBuffer::skipBytes(Offset distance, Log* log)
{
    for (;;) {
        size_t thisTime = (size_t)Min<Offset>(getBytesAvailable(), distance);

        _ptr += thisTime;
        distance -= (Offset)thisTime;

        if (!distance) {
            return true;
        }

        if (fetchMore(log) <= 0) {
            return false;
        }
    }
}

bool StreamBuffer::skipMatchingBytes(bool all, bool inSet, StringView set, Log* log)
{
    for (;;) {
        int c = peekByte(log);
        if (c < 0) {
            return !getErrorFlag();
        }

        bool in = std::find(set.begin(), set.end(), (char)c) != set.end();

        if (in) {
            if (!inSet) {
                return true;
            }
        } else {
            if (inSet) {
                return true;
            }
        }

        skipByte();
        if (!all) {
            return true;
        }
    }
}

bool StreamBuffer::readBytes(void* memory, size_t byteCount, Log* log)
{
    if (!byteCount) {
        return true;
    }

    // TODO: if reading in excess of getBufferSize(), read directly in to the destination memory.

    for (;;) {
        size_t take = byteCount < getBytesAvailable() ? byteCount : getBytesAvailable();
        memcpy(memory, _ptr, take);
        _ptr += take;
        byteCount -= take;
        memory = (char*)memory + take;

        if (!byteCount) {
            break;
        }

        if (fetchMore(log) <= 0) {
            return false;
        }
    }

    return true;
}

ptrdiff_t StreamBuffer::requestNumberOfBytes(size_t byteCount, Log* log)
{
    while (getBytesAvailable() < byteCount) {
        ptrdiff_t fetched = fetchMore(log);
        if (fetched < 0) {
            return -1;
        }
        if (fetched == 0) {
            break;
        }
    }

    return getBytesAvailable();
}

ptrdiff_t StreamBuffer::find(const void* string, size_t stringSize, Log* log)
{
    return pointerToIndex(findPointer(string, stringSize, log));
}

const char* StreamBuffer::findPointer(const void* string, size_t stringSize, Log* log)
{
    if (!PRIME_GUARD(stringSize)) {
        return _ptr;
    }

    char string0 = *(const char*)string;
    const char* ptr = _ptr;

    for (;;) {
        while ((size_t)(_top - ptr) < stringSize) {
            ptrdiff_t ptrOffset = ptr - _ptr;
            ptrdiff_t fetched = fetchMore(log);
            if (fetched < 0) {
                return NULL;
            }

            if (fetched == 0) {
                return _top;
            }

            ptr = _ptr + ptrOffset;
        }

        for (; ptr <= _top - stringSize; ++ptr) {
            if (*ptr == string0) {
                if (memcmp(ptr, string, stringSize) == 0) {
                    return ptr;
                }
            }
        }
    }
}

ptrdiff_t StreamBuffer::findFirstOf(const void* string, size_t stringSize, Log* log)
{
    return pointerToIndex(findFirstOfPointer(string, stringSize, log));
}

const char* StreamBuffer::findFirstOfPointer(const void* string, size_t stringSize, Log* log)
{
    if (!PRIME_GUARD(stringSize)) {
        return _ptr;
    }

    const char* ptr = _ptr;

    for (;;) {
        while ((size_t)(_top - ptr) < 1) {
            ptrdiff_t ptrOffset = ptr - _ptr;

            ptrdiff_t fetched = fetchMore(log);
            if (fetched < 0) {
                return NULL;
            }

            ptr = _ptr + ptrOffset;

            if (fetched == 0) {
                return _top;
            }
        }

        for (; ptr != _top; ++ptr) {
            if (memchr(string, (int)(uint8_t)*ptr, stringSize)) {
                return ptr;
            }
        }
    }
}

bool StreamBuffer::readLine(char* buffer, size_t bufferSize, Log* log, char** newlinePointer)
{
    if (!PRIME_GUARD(bufferSize)) {
        return false;
    }

    static const char newlines[2] = { '\r', '\n' };

    const char* ptr = findFirstOfPointer(newlines, COUNTOF(newlines), log);
    if (!ptr) {
        return false;
    }

    if (ptr == _top) {
        // fetchMore() can move the buffer around, so recompute ptr.
        size_t ptrIndex = ptr - _ptr;
        if (fetchMore(log) < 0) {
            return false;
        }

        ptr = _ptr + ptrIndex;
    }

    const char* next = ptr;
    if (ptr != _top) {
        ++next;
        if (*ptr == '\r' && *next == '\n') {
            ++next;
        } else if (*ptr == '\n' && *next == '\r') {
            ++next;
        }
    }

    if (newlinePointer) {
        size_t lineSize = (size_t)(ptr - _ptr);

        if (lineSize < bufferSize) {
            *newlinePointer = buffer + lineSize;
        } else {
            *newlinePointer = &buffer[bufferSize - 1];
        }
    }

    size_t size = (size_t)(next - _ptr);
    size_t copySize = Min(bufferSize - 1, size);

    memcpy(buffer, _ptr, copySize);
    _ptr += copySize;
    buffer[copySize] = 0;

    return true;
}

void StreamBuffer::setUnderlyingStream(Stream* stream, Offset offset)
{
    PRIME_ASSERT(isEmpty());
    _ptr = _top = _buffer;
    _underlyingStream = stream;

    if (offset < 0) {
        _underlyingOffset = _underlyingStream->getOffset(Log::getNullLog());
        PRIME_ASSERT(_underlyingOffset >= 0); // You must supply an offset >= 0 for streams which don't support seeking.
        if (_underlyingOffset < 0) {
            _underlyingOffset = 0;
        }
    } else {
        _underlyingOffset = offset;
    }

    _bufferOffset = _underlyingOffset;
}

bool StreamBuffer::readLine(std::string& string, Log* log, std::string::size_type* newlineOffset, std::string::size_type maxLength)
{
    char* ptr = _ptr;

    string.resize(0);

    for (;;) {
        // Do this at the top of the loop in case we happened to be writing.
        if (ptr == _top) {
            if (ptr != _ptr) {
                string.append(_ptr, ptr);
                _ptr = ptr;
            }

            ptrdiff_t got = fetchMore(log);

            if (got < 0) {
                return false;
            }

            if (got == 0) {
                if (newlineOffset) {
                    *newlineOffset = string.size();
                }

                return true;
            }

            ptr = _ptr;
        }

        const char* top;

        if (maxLength) {
            size_t available = (size_t)(_top - ptr);
            size_t space = string.size() > maxLength ? 0 : maxLength - string.size();

            top = _ptr + Min(available, space);
        } else {
            top = _top;
        }

        while (ptr != top && *ptr != '\r' && *ptr != '\n') {
            ++ptr;
        }

        if (maxLength && ptr == top) {
            // We've reached maxLength.
            string.append(_ptr, ptr);
            _ptr = ptr;

            if (newlineOffset) {
                *newlineOffset = string.size();
            }

            return true;
        }

        if (ptr == _top) {
            // We've scanned the entire buffer.
            continue;
        }

        char newline = *ptr++;

        string.append(_ptr, ptr);
        _ptr = ptr;

        if (newlineOffset) {
            *newlineOffset = string.size() - 1;
        }

        // This will block if reading from a socket where there is no trailing character, making it unusable
        // for sockets.
        if (ptr == _top) {
            ptrdiff_t got = fetchMore(log);

            if (got < 0) {
                return false;
            }
        }

        if (_ptr != _top) {
            if ((newline == '\r' && *_ptr == '\n') || (newline == '\n' && *_ptr == '\r')) {
                string += *_ptr++;
            }
        }

        return true;
    }
}

bool StreamBuffer::readNullTerminated(std::string& string, Log* log, std::string::size_type maxLength,
    bool* reachedMaxLength)
{
    char* ptr = _ptr;

    string.resize(0);

    if (reachedMaxLength) {
        *reachedMaxLength = false;
    }

    for (;;) {
        // Do this at the top of the loop in case we happened to be writing.
        if (ptr == _top) {
            if (ptr != _ptr) {
                string.append(_ptr, ptr);
                _ptr = ptr;
            }

            ptrdiff_t got = fetchMore(log);

            if (got < 0) {
                return false;
            }

            if (got == 0) {
                return true;
            }

            ptr = _ptr;
        }

        const char* top;

        if (maxLength) {
            size_t available = (size_t)(_top - ptr);
            size_t space = string.size() > maxLength ? 0 : maxLength - string.size();

            top = _ptr + Min(available, space);
        } else {
            top = _top;
        }

        while (ptr != top && *ptr != 0) {
            ++ptr;
        }

        if (maxLength && ptr == top) {
            // We've reached maxLength.
            if (!reachedMaxLength) {
                log->error(PRIME_LOCALISE("Null terminated string exceeded maximum length."));
                return false;
            }

            if (reachedMaxLength) {
                *reachedMaxLength = true;
            }

            string.append(_ptr, ptr);
            _ptr = ptr;

            return true;
        }

        if (ptr == _top) {
            // We've scanned the entire buffer.
            continue;
        }

        string.append(_ptr, ptr);
        ++ptr;
        _ptr = ptr;

        return true;
    }
}
}
