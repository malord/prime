// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STDIOSTREAM_H
#define PRIME_STDIOSTREAM_H

#include "Stream.h"
#include <stdio.h>

namespace Prime {

/// A Stream wrapper around a stdio FILE *.
class PRIME_PUBLIC StdioStream : public Stream {
    PRIME_DECLARE_UID_CAST(Stream, 0x1c8ec12b, 0x87e64e0f, 0x8e91fbb0, 0x09d5ba18)

public:
    typedef FILE* Handle;

    StdioStream();

    explicit StdioStream(FILE* fp, bool closeWhenDone = true);

    explicit StdioStream(const char* filename, const char* fopenMode, Log* log,
        const OpenMode& openMode = OpenMode());

    ~StdioStream();

    /// Returns false if the last FILE couldn't be closed.
    bool attach(FILE* fp, bool closeWhenDone = true);

    /// Detach the file handle from this object.
    FILE* detach();

    bool fopen(const char* filename, const char* fopenMode, Log* log,
        const OpenMode& openMode = OpenMode());

    /// Compatible with FileStream.
    bool open(const char* filename, const OpenMode& openMode, Log* log);

    bool openForRead(const char* filename, Log* log)
    {
        return open(filename, OpenMode().setRead(), log);
    }

    bool openForWrite(const char* filename, Log* log)
    {
        return open(filename, OpenMode().setOverwrite(), log);
    }

    /// Returns true if we have a file descriptor.
    bool isOpen() const { return _fp != 0; }

    FILE* getHandle() const { return _fp; }

    int getFileNo() const { return fileno(_fp); }

    /// Returns true if we're not open.
    bool operator!() const { return _fp == 0; }

    /// Direct access to the FILE *.
    FILE* getFile() const { return _fp; }

    /// Detach the FILE from this object and return it.
    FILE* detachFile()
    {
        FILE* detached = _fp;
        zero();
        return detached;
    }

    /// Set the FILE to binary mode (for platforms where \n isn't the newline).
    void setBinaryMode();

    /// Set the FILE to text mode (for platforms where \n isn't the newline).
    void setTextMode();

    // Stream implementation
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readSome(void* buffer, size_t maxBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeSome(const void* bytes, size_t maxBytes, Log* log) PRIME_OVERRIDE;
    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE;
    virtual Offset getSize(Log* log) PRIME_OVERRIDE;
    virtual bool setSize(Offset newSize, Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;

private:
    void construct() { zero(); }

    void zero()
    {
        _fp = 0;
        _shouldClose = false;
    }

    FILE* _fp;
    bool _shouldClose;

    PRIME_UNCOPYABLE(StdioStream);
};
}

#endif
