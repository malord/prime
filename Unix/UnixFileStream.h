// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXFILESTREAM_H
#define PRIME_UNIX_UNIXFILESTREAM_H

#include "../Stream.h"

namespace Prime {

/// A wrapper around a UNIX file descriptor.
class PRIME_PUBLIC UnixFileStream : public Stream {
    PRIME_DECLARE_UID_CAST(Stream, 0xe2fc2288, 0x93344ea2, 0xb9fead65, 0x02154672)

public:
    typedef int Handle;

    UnixFileStream();

    ~UnixFileStream();

    /// Open a file using a combination of OpenFlags (e.g., openRead).
    bool open(const char* path, const OpenMode& openMode, Log* log);

    bool openForRead(const char* filename, Log* log)
    {
        return open(filename, OpenMode().setRead(), log);
    }

    bool openForWrite(const char* filename, Log* log)
    {
        return open(filename, OpenMode().setOverwrite(), log);
    }

    /// Open a file using UNIX open(2) flags.
    bool unixOpen(const char* path, int unixOpenFlags, Log* log, const OpenMode& openMode = OpenMode());

    /// Attach a UNIX file handle to this object, closing any previous handle.
    void attach(int existingHandle, bool closeWhenDone = true, bool syncOnClose = false);

    /// Detach the file handle from this object.
    int detach();

    /// Returns true if we have a file descriptor.
    bool isOpen() const { return _handle >= 0; }

    int getHandle() const { return _handle; }

    /// getHandle() and getFileNo() are distinct concepts for some streams (e.g., StdioStream).
    int getFileNo() const { return _handle; }

    /// Returns false if an error occurs.
    virtual bool close(Log* log) PRIME_OVERRIDE;

    virtual ptrdiff_t readSome(void* buffer, size_t maxBytes, Log* log) PRIME_OVERRIDE;

    virtual ptrdiff_t writeSome(const void* bytes, size_t maxBytes, Log* log) PRIME_OVERRIDE;

    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE;

    virtual Offset getSize(Log* log) PRIME_OVERRIDE;

    virtual bool setSize(Offset newSize, Log* log) PRIME_OVERRIDE;

    virtual bool flush(Log* log) PRIME_OVERRIDE;

private:
    int _handle;
    bool _shouldClose;
    bool _syncOnClose;

    PRIME_UNCOPYABLE(UnixFileStream);
};
}

#endif
