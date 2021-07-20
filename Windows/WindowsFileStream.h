// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSFILESTREAM_H
#define PRIME_WINDOWS_WINDOWSFILESTREAM_H

#include "../Stream.h"
#include "WindowsConfig.h"

namespace Prime {

/// A wrapper around a Windows file handle.
class PRIME_PUBLIC WindowsFileStream : public Stream {
    PRIME_DECLARE_UID_CAST(Stream, 0xb431f154, 0xcb724477, 0x874f844c, 0x9f65cd57)

public:
    typedef HANDLE Handle;

    WindowsFileStream();

    ~WindowsFileStream();

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

    /// Open a file using CreateFile.
    bool windowsOpen(const char* path, DWORD access, DWORD shareMode, LPSECURITY_ATTRIBUTES sa,
        DWORD creationDisposition, DWORD flagsAndAttributes, Log* log,
        const OpenMode& openMode = OpenMode());

    /// Attach a Windows file handle to this object, closing any previous handle.
    void attach(HANDLE handle, bool closeWhenDone = true);

    /// Detach the file handle from this object.
    HANDLE detach();

    /// Returns true if we have a file descriptor.
    bool isOpen() const { return _handle != INVALID_HANDLE_VALUE; }

    HANDLE getHandle() const { return _handle; }

    /// Returns false if the file wasn't closed successfully.
    bool close(Log* log);

    virtual ptrdiff_t readSome(void* buffer, size_t maxBytes, Log* log) PRIME_OVERRIDE;

    virtual ptrdiff_t writeSome(const void* bytes, size_t maxBytes, Log* log) PRIME_OVERRIDE;

    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE;

    virtual Offset getSize(Log* log) PRIME_OVERRIDE;

    virtual bool setSize(Offset newSize, Log* log) PRIME_OVERRIDE;

    virtual bool flush(Log* log) PRIME_OVERRIDE;

private:
    HANDLE _handle;
    bool _shouldClose;

    PRIME_UNCOPYABLE(WindowsFileStream);
};
}

#endif
