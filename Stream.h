// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STREAM_H
#define PRIME_STREAM_H

#include "Log.h"
#include "OpenMode.h"
#include "StringView.h"

namespace Prime {

/// Generic byte-oriented I/O interface.
class PRIME_PUBLIC Stream : public RefCounted {
    PRIME_DECLARE_UID_CAST_BASE(0xfeba04a8, 0xbe02411e, 0xa397a811, 0x8f84f73a)

public:
    /// Offset within a Stream.
    typedef int64_t Offset;

#define PRIME_PRId_STREAM PRId64

    Stream()
    {
    }

    virtual ~Stream() { }

    /// Close the file and verify success. This is optional, the file will be closed for you when the Stream is
    /// released, but release doesn't provide error information. On error, returns false and logs an error.
    /// If the Stream writes to an underlying Stream this will also close the underlying Stream, so you should
    /// only ever call close() on Streams that you opened.
    virtual bool close(Log* log);

    /// Read at least one byte from the stream and at most maxBytes. Returns the number of bytes read, 0 at the
    /// end of the file, -1 on error. Errors are logged.
    virtual ptrdiff_t readSome(void* buffer, size_t maxBytes, Log* log);

    /// Write at least one byte to the stream and at most maxBytes. Returns the number of bytes written, 0 if the
    /// backing store is full (or the connection has been closed), -1 on error. Errors are logged.
    virtual ptrdiff_t writeSome(const void* bytes, size_t maxBytes, Log* log);

    /// Read a specific number of bytes at a specified offset. Returns < requiredBytes only if the end of the
    /// file is reached. Returns < 0 on error.
    virtual ptrdiff_t readAtOffset(Offset offset, void* buffer, size_t requiredBytes, Log* log);

    /// Write a specific number of bytes at a specific offset. Returns < requiredBytes only if the disk is full.
    /// Returns < 0 on error.
    virtual ptrdiff_t writeAtOffset(Offset offset, const void* bytes, size_t byteCount, Log* log);

    enum SeekMode {
        SeekModeAbsolute,
        SeekModeRelative,
        SeekModeRelativeToEnd
    };

    /// Seek within the stream and return the new offset. Returns -1 on error. Not all streams will be seekable,
    /// but all will support skipping (with the skip() method) and as many as possible should support rewind().
    virtual Offset seek(Offset offset, SeekMode mode, Log* log);

    /// Returns the size, in bytes, of the stream or -1 if not known. Do not report an error if the Stream does
    /// not know its size (but do report an actual error).
    virtual Offset getSize(Log* log);

    /// Set the size of the backing store (e.g., the file). Returns true on success, false if setting the size
    /// isn't supported or an error occurs. Errors are logged.
    virtual bool setSize(Offset newSize, Log* log);

    /// Flush any buffered data, return false on error. Automatically called for you by close() and release().
    /// Errors are logged.
    virtual bool flush(Log* log);

    /// Copy bytes to this Stream from another. Can be overridden to allow more optimal copying methods to be
    /// used (e.g., zero-copy file to socket transfers, or decompression directly to an output file).
    virtual bool copyFrom(Stream* source, Log* sourceLog, Offset length, Log* destLog, size_t bufferSize = 0,
        void* buffer = NULL);

    /// The default copyFrom implementation calls this first to see if a fast copy can be performed the other
    /// way. A decompressor could use this to efficiently decompress without worrying about reading a block at
    /// a time.
    virtual bool tryCopyTo(bool& error, Stream* dest, Log* destLog, Offset length, Log* sourceLog,
        size_t bufferSize = 0, void* buffer = NULL);

    /// If this Stream wraps another, return the other Stream. This is optional.
    virtual Stream* getUnderlyingStream() const;

    /// Return true if this Stream is seekable. The default implementation checks the return value of getOffset().
    virtual bool isSeekable();

    //
    // Helpers
    //

    /// Read as many bytes as possible up to maxBytes or the end of the stream, whichever is first. This emulates
    /// the behaviour of POSIX read(). Returns -1 on error, maxBytes on success, < maxBytes if the end of the
    /// stream was found.
    ptrdiff_t read(void* buffer, size_t maxBytes, Log* log);

    /// Write as many bytes as possible up to maxBytes or the maximum capacity of the stream, whichever is first.
    /// This emulates the behaviour of POSIX write(). Returns -1 on error, maxBytes on success, < maxBytes if the
    /// backing store is full or the connection was closed.
    ptrdiff_t write(const void* bytes, size_t maxBytes, Log* log);

    /// Read the exact number of bytes requested. If an error occurs or the end of the stream is reached, logs an
    /// error and returns false.
    bool readExact(void* buffer, size_t byteCount, Log* log, const char* errorMessage = NULL);

    /// Read the exact number of bytes requested form the specified offset. If an error occurs or the end of the
    /// stream is reached, logs an error and returns false.
    bool readExact(Offset offset, void* buffer, size_t byteCount, Log* log, const char* errorMessage = NULL);

    /// Write the exact number of bytes specified. If an error occurrs or the backing store is filled, logs an
    /// error and returns false.
    bool writeExact(const void* bytes, size_t byteCount, Log* log, const char* errorMessage = NULL);

    /// Write the exact number of bytes requested at the specified offset. If an error occurrs or the backing
    /// store is filled, logs an error and returns false.
    bool writeExact(Offset offset, const void* bytes, size_t byteCount, Log* log, const char* errorMessage = NULL);

    /// Seek to an exact position and return true on success, false on error.
    bool setOffset(Stream::Offset offset, Log* log);

    /// Seek back to the start. Streams should implement this wherever possible.
    bool rewind(Log* log) { return setOffset(0, log); }

    /// Returns the current offset within the stream, -1 if not known.
    Offset getOffset(Log* log) { return seek(0, SeekModeRelative, log); }

    /// Jump forward within the stream by reading. Returns false on error.
    bool skip(Offset distance, Log* log, const char* errorMessage = NULL);

    bool printf(Log* log, const char* format, ...);

    bool vprintf(Log* log, const char* format, va_list argptr);

    Stream* getMostUnderlyingStream() const;

    bool writeString(StringView string, Log* log);

protected:
    static bool copy(Stream* dest, Log* destLog, Stream* source, Log* sourceLog, Offset length, size_t bufferSize,
        void* buffer);

private:
    PRIME_UNCOPYABLE(Stream);
};

/// Write data to a file using any Stream type which has an openForWrite method.
template <typename Stream>
bool SaveFile(const char* path, const void* data, size_t sizeInBytes, Log* log)
{
    Stream stream;
    if (!stream.openForWrite(path, log)) {
        return false;
    }

    if (!stream.writeExact(data, sizeInBytes, log)) {
        return false;
    }

    return stream.close(log);
}
}

#endif
