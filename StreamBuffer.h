// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STREAMBUFFER_H
#define PRIME_STREAMBUFFER_H

#include "ScopedPtr.h"
#include "Stream.h"
#include "StringView.h"

namespace Prime {

/// A read/write buffer for a Stream. Only seeks the underlying Stream if you alternate reads/writes or explicitly
/// seek().
class PRIME_PUBLIC StreamBuffer : public Stream {
    PRIME_DECLARE_UID_CAST(Stream, 0xd83520f9, 0x03c74307, 0xaff483b2, 0x67ebd141)

public:
    //
    // Construction
    //

    StreamBuffer();

    StreamBuffer(Stream* underlyingStream, size_t bufferSize, void* buffer = NULL);

    /// Constructs a read-only StreamBuffer which reads from the supplied bytes.
    StreamBuffer(const void* bytes, size_t byteCount);

    ~StreamBuffer();

    //
    // Initialisation
    //

    bool init(Stream* underlyingStream, size_t bufferSize, void* buffer = NULL);

    /// Initialise as read-only to read the specified bytes directly.
    void init(const void* bytes, size_t byteCount);

    //
    // Stream implementation
    //

    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readSome(void* buffer, size_t maxBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeSome(const void* bytes, size_t maxBytes, Log* log) PRIME_OVERRIDE;
    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE;
    virtual Offset getSize(Log* log) PRIME_OVERRIDE;
    virtual bool setSize(Offset newSize, Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;

    //
    // Buffer control
    //

    /// Flush any buffered writes. Returns false on error. Does not empty the buffer.
    bool flushWrites(Log* log);

    /// Flush any unwritten data then clear the buffer. If we've been reading from the underlying stream then
    /// its offset may be beyond our offset. If you're going to seek to an absolute offset immediately after this
    /// then it's not a problem, but if not then the next read/write will occur at the offset we were at in the
    /// underlying stream, and not the offset we emulated. If you set seekBack to true, the underlying stream
    /// will be seek'd to our offset, keeping the offsets in sync.
    bool unbuffer(bool seekBack, Log* log);

    /// This can only be called after init() has been called and when the buffer is empty. Switches to a
    /// different underlying Stream.
    void setUnderlyingStream(Stream* stream, Offset offset = 0);

    virtual Stream* getUnderlyingStream() const PRIME_OVERRIDE { return _underlyingStream; }

    Offset getUnderlyingStreamOffset() const { return _underlyingOffset; }

    //
    // Configuration
    //

    /// Set the maximum number of bytes that may be put back in to the buffer. The default is 1. This ensures that
    /// when the contents of the buffer are shifted to make room, at least this number of bytes remain.
    void setMaxPutBack(size_t putback) PRIME_NOEXCEPT { _maxPutBack = putback; }

    /// Returns the maximum number of bytes that can be put back.
    size_t getMaxPutBack() const PRIME_NOEXCEPT { return _maxPutBack; }

    //
    // Buffer state
    //

    size_t getBufferSize() const PRIME_NOEXCEPT { return (size_t)(_end - _buffer); }

    bool isFull() const PRIME_NOEXCEPT { return _ptr == _buffer && _top == _end; }

    bool isEmpty() const PRIME_NOEXCEPT { return _ptr == _top; }

    /// The error flag is set if the underlying Stream encountered an error which we could not return. If set,
    /// close() will return error.
    bool getErrorFlag() const PRIME_NOEXCEPT { return _error; }

    bool isDirty() const PRIME_NOEXCEPT { return _dirtyEnd > _dirtyBegin; }

    bool isReadOnly() const PRIME_NOEXCEPT { return _const; }

    //
    // Writing
    //

    /// Write a byte. This is not an efficient way to use StreamBuffer - instead, use makeSpace(), then write to
    /// the returned pointer, then use advanceWritePointer() to record the write.
    bool writeByte(int c, Log* log);

    /// Write an array of bytes. Returns false on error.
    bool writeBytes(const void* memory, size_t size, Log* log);

    /// Returns the amount of space available in the buffer.
    size_t getSpace() const PRIME_NOEXCEPT { return (size_t)(_end - _ptr); }

    /// Make room for the specified number of bytes in the stream. Returns a write pointer if the space is
    /// available, null otherwise.
    void* makeSpace(size_t needed, Log* log)
    {
        if (getSpace() < needed) {
            return makeSpaceBufferFull(needed, log);
        }

        return NULL;
    }

    /// Return the write pointer, which you can write in to directly. After calling this you must call
    /// advanceWritePointer() and specify how many bytes you wrote.
    void* getWritePointer() const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(getBufferSize());
        PRIME_DEBUG_ASSERT(!isReadOnly());

        return _ptr;
    }

    /// Advance the write pointer.
    void advanceWritePointer(size_t byteCount) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(byteCount <= getSpace());
        PRIME_DEBUG_ASSERT(!isReadOnly());

        if (_ptr < _dirtyBegin) {
            _dirtyBegin = _ptr;
        }
        _ptr += byteCount;
        if (_ptr > _dirtyEnd) {
            _dirtyEnd = _ptr;
        }
        if (_ptr > _top) {
            _top = _ptr;
        }
    }

    //
    // Reading
    //

    /// Returns the number of bytes currently available to read.
    size_t getBytesAvailable() const PRIME_NOEXCEPT { return (size_t)(_top - _ptr); }

    /// Move the read pointer back specified number of bytes.
    void putBack(size_t byteCount) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(byteCount <= (size_t)(_ptr - _buffer));
        _ptr -= byteCount;
    }

    /// Ensure the specified number of bytes are present in the buffer, fetching more if necessary. Returns null
    /// if there aren't enough bytes remaining in the stream.
    const char* requireNumberOfBytes(size_t byteCount, Log* log)
    {
        if (getBytesAvailable() < byteCount) {
            return requireNumberOfBytesRefillBuffer(byteCount, log);
        }

        return getReadPointer();
    }

    /// Like requireNumberOfBytes(), but returns the number of bytes available, which may be less than requested
    /// if the end of the file has been reached. Returns < 0 on error.
    ptrdiff_t requestNumberOfBytes(size_t byteCount, Log* log);

    /// Read one or more bytes in to the buffer. Returns the number of bytes that were read, < 0 on error. If
    /// there is no room in the buffer, returns 0 (this can be detected by calling isFull()). If there are
    /// no more bytes available to read from the underlying Stream, also returns 0.
    /// Note that since fetchMore() can shuffle the buffer, any pointers in to the buffer are invalid after
    /// calling this method.
    ptrdiff_t fetchMore(Log* log);

    /// Load as many bytes as possible in to the buffer. Returns < 0 on error, otherwise returns the number of
    /// bytes now available to read.
    ptrdiff_t fetchUntilBufferIsFull(Log* log)
    {
        return requestNumberOfBytes(getBufferSize(), log);
    }

    /// Read the next character from the buffer. Returns < 0 on error, including end-of-file. You can detect
    /// error as opposed to end-of-file by calling getErrorFlag().
    int readByte(Log* log)
    {
        if (_ptr == _top) {
            return readByteRefillBuffer(log);
        }

        return (int)(uint8_t)(*_ptr++);
    }

    /// Read the next character from the buffer and store it in *ch. Returns true on success, false on error.
    /// You can detect error as opposed to end-of-file by calling getErrorFlag().
    bool readByte(uint8_t& ch, Log* log)
    {
        if (_ptr == _top) {
            return readByteRefillBuffer(ch, log);
        }

        ch = (uint8_t)*_ptr++;
        return true;
    }

    /// Read the specified number of bytes. Use this instead of Stream::read().
    bool readBytes(void* memory, size_t byteCount, Log* log);

    /// Read the next character in the buffer without consuming it. Returns < 0 on error.
    int peekByte(Log* log)
    {
        if (_ptr == _top) {
            return peekByteRefillBuffer(0, log);
        }

        return (int)(uint8_t)(*_ptr);
    }

    /// Peek a character in the buffer without consuming it. Returns < 0 on error.
    int peekByte(size_t offset, Log* log)
    {
        if (_ptr + offset >= _top) {
            return peekByteRefillBuffer(offset, log);
        }

        return (int)(uint8_t)_ptr[offset];
    }

    /// Peek a character in the buffer, returning false if the index is too high.
    bool peekByte(size_t offset, uint8_t& ch, Log* log)
    {
        if (_ptr + offset >= _top) {
            return peekByteRefillBuffer(offset, ch, log);
        }

        ch = (uint8_t)_ptr[offset];
        return true;
    }

    /// Read the specified number of bytes from the start of the buffer (fetching more in to the buffer if
    /// necessary), without advancing the read pointer.
    bool peekBytes(void* memory, size_t byteCount, Log* log);

    /// Read the specified number of bytes from the requested offset in the buffer (fetching more in to the
    /// buffer if necessary), without advancing the read pointer.
    bool peekBytes(size_t offset, void* memory, size_t byteCount, Log* log);

    /// Returns true if the next bytes to be read from the buffer will match the supplied array. The buffer size
    /// must be >= byteCount.
    bool matchBytes(const void* bytes, size_t byteCount, Log* log);

    /// Returns true if the bytes in the buffer beginning at the specified offset match the supplied array. The
    /// buffer size must be >= byteCount + offset.
    bool matchBytes(size_t offset, const void* bytes, size_t byteCount, Log* log);

    /// Skip matching bytes in the file. Returns false if the contents of the file don't match, or on error.
    bool skipMatchingBytes(const void* bytes, size_t byteCount, Log* log);

    /// Skips all (or first) occurrences of any characters in/not-in the supplied set.
    bool skipMatchingBytes(bool all, bool inSet, StringView set, Log* log);

    /// Skip a single byte in the buffer. There must be a byte in the buffer to skip.
    void skipByte() PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!isEmpty());
        ++_ptr;
    }

    /// Jump forward within the stream by reading. Returns false on error.
    bool skipBytes(Offset distance, Log* log);

    /// Jump forward within the stream by reading. Returns false on error. Named skip() to intentionally hide
    /// Stream::skip so skipBytes() gets used.
    bool skip(Offset distance, Log* log)
    {
        return skipBytes(distance, log);
    }

    /// Returns the read pointer, which points to the next byte that would be read.
    const char* getReadPointer() const PRIME_NOEXCEPT { return _ptr; }

    /// Returns the "top" pointer, which points to the byte after the last byte in the buffer, i.e.,
    /// getTopPointer() - getReadPointer() returns the number of bytes in the buffer.
    const char* getTopPointer() const PRIME_NOEXCEPT { return _top; }

    /// Skip the specified number of bytes in the buffer. There must be sufficient bytes in the buffer to skip,
    /// otherwise you should use skipBytes(), which will read more (and skip them) as necessary.
    void advanceReadPointer(size_t byteCount) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(byteCount <= getBytesAvailable());
        _ptr += byteCount;
    }

    /// Move the read pointer within the buffer.
    void setReadPointer(const void* newPointer) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT((const char*)newPointer >= _buffer && newPointer <= _top);
        _ptr = (char*)newPointer;
    }

    //
    // Searching
    //

    /// Search the buffer for the specified string until the buffer is filled. Returns the offset of the match,
    /// or -1 on error or if there is no match.
    ptrdiff_t find(const void* string, size_t stringSize, Log* log);

    /// Search the buffer for the specified string until the buffer is filled. Returns a pointer to the match,
    /// or to the end of the buffer if no match is found. Returns NULL on error.
    const char* findPointer(const void* string, size_t stringSize, Log* log);

    /// Search the buffer for any of the bytes in string. Returns the offset of the match, or -1 if an error
    /// occurred or if there is no match.
    ptrdiff_t findFirstOf(const void* string, size_t stringSize, Log* log);

    /// Search the buffer for any of the bytes in string. Returns a pointer to the match, or to the end if no
    /// match is found. Returns NULL on error.
    const char* findFirstOfPointer(const void* string, size_t stringSize, Log* log);

    //
    // Reading lines
    //

    /// Read a line in to a buffer, including the terminator (which can be "\n", "\r\n", "\r" or "\n\r").
    /// Returns false on error. At the end of the file, *buffer will be '\0'. If the newline didn't fit in the
    /// buffer, *newlinePointer will point to the terminating '\0' in the buffer, so it is always safe to do
    /// `*newlinePointer = '\0'` to strip the newline from a line.
    bool readLine(char* buffer, size_t bufferSize, Log* log, char** newlinePointer = NULL);

    /// Read a line in to a std::string, including the newline sequence. Returns false on error, yields an empty
    /// string at the end of the file. maxLength can be set to avoid denial of service attacks caused by sending
    /// unterminated lines.
    bool readLine(std::string& string, Log* log, std::string::size_type* newlineOffset = NULL,
        std::string::size_type maxLength = 0);

    /// Read until a nul terminator or until maxLength bytes have been read, if maxLength > 0. If reachedMaxLength
    /// is NULL and maxLength is set, it is an error to read more than maxLength bytes and an error will be
    /// emitted and false returned. If reachedMaxLength is not null, it will be set to true if maxLength was
    /// reached before a nul byte.
    bool readNullTerminated(std::string& string, Log* log, std::string::size_type maxLength = 0,
        bool* reachedMaxLength = NULL);

private:
    ptrdiff_t pointerToIndex(const void* pointer) const
    {
        return pointer ? (const char*)pointer - _ptr : -1;
    }

    bool shift(Log* log);

    bool writeByteBufferFull(int c, Log* log);

    void* makeSpaceBufferFull(size_t space, Log* log);

    const char* requireNumberOfBytesRefillBuffer(size_t byteCount, Log* log);

    int readByteRefillBuffer(Log* log);

    bool readByteRefillBuffer(uint8_t& ch, Log* log);

    int peekByteRefillBuffer(size_t offset, Log* log);

    bool peekByteRefillBuffer(size_t offset, uint8_t& ch, Log* log);

    void zero();

    RefPtr<Stream> _underlyingStream;
    Offset _underlyingOffset;
    ScopedArrayPtr<char> _allocatedBuffer;
    char* _buffer;

    char* _top;
    char* _ptr;
    char* _end;
    char* _dirtyBegin;
    char* _dirtyEnd;
    Stream::Offset _bufferOffset;
    bool _seekable;

    size_t _maxPutBack;
    bool _error;

    bool _const;

    PRIME_UNCOPYABLE(StreamBuffer);
};
}

#endif
