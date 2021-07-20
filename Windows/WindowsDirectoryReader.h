// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSDIRECTORYREADER_H
#define PRIME_WINDOWS_WINDOWSDIRECTORYREADER_H

#include "../DirectoryReaderBase.h"
#include "../Log.h"
#include "../UnixTime.h"
#include "WindowsConfig.h"

namespace Prime {

/// Wrapper around the FindFirstFile/FindNextFile APIs. WindowsDirectoryReader is used for both the
/// DirectoryReader typedef and the FileProperties typedef on Windows. A WindowsDirectoryReader can be assigned
/// to another instance, but only the file properties are copied.
class PRIME_PUBLIC WindowsDirectoryReader : public DirectoryReaderBase {
public:
    WindowsDirectoryReader();

    WindowsDirectoryReader(const WindowsDirectoryReader& copy)
    {
        operator=(copy);
    }

    ~WindowsDirectoryReader();

    bool isInitialised() const { return _handle != INVALID_HANDLE_VALUE; }

    /// Open a directory for reading. "path" is the path to a directory and cannot contain a wildcard.
    bool open(const char* path, Log* log, const Options& options = Options());

    /// Wrapper around FindFirstFile. "wildcard" can contain a Windows wildcard.
    bool openWildcard(const char* wildcard, Log* log, const Options& options = Options());

    /// Direct wrapper around FindFirstFile. "wildcard" can contain a Windows wildcard.
    bool openTCharWildcard(LPCTSTR wildcard, Log* log, const Options& options = Options());

    bool isOpen() const { return _handle != INVALID_HANDLE_VALUE || _first; }

    void close();

    /// Read the next directory entry. Returns false if there are no more entries to read.
    bool read(Log* log, bool* error = NULL);

    /// This method should only be called after a successful call to read().
    const WIN32_FIND_DATA& getWin32FindData() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return _data;
    }

    /// This method should only be called after a successful call to read().
    WIN32_FIND_DATA& getWin32FindData()
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return _data;
    }

    // DEPRECATED! operator -> will be removed at some point

    /// This method should only be called after a successful call to read().
    PRIME_DEPRECATED const WIN32_FIND_DATA* operator->() const { return &getWin32FindData(); }

    WIN32_FIND_DATA* operator->() { return &getWin32FindData(); }

    UnixTime getCreationTime() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return UnixTime::fromWindowsFileTime(_data.ftCreationTime);
    }

    UnixTime getLastAccessTime() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return UnixTime::fromWindowsFileTime(_data.ftLastAccessTime);
    }

    UnixTime getLastWriteTime() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return UnixTime::fromWindowsFileTime(_data.ftLastWriteTime);
    }

    UnixTime getModificationTime() const { return getLastWriteTime(); }

    int64_t getSize() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return PRIME_MAKE64(_data.nFileSizeLow, _data.nFileSizeHigh);
    }

    DWORD getWindowsAttributes() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return _data.dwFileAttributes;
    }

    bool isDirectory() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return (_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

#ifdef FILE_ATTRIBUTE_DEVICE
    bool isDevice() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return (_data.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) != 0;
    }
#endif

    bool isHidden() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return (_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
    }

    bool isReadOnly() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return (_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
    }

    bool isSystem() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return (_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
    }

    bool isLink() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return (_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    }

    bool isFile() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
#ifdef FILE_ATTRIBUTE_DEVICE
        return (_data.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_DIRECTORY)) == 0;
#else
        return (_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
#endif
    }

    /// Returns the file name, without path, of the directory entry.
    const char* getName() const
    {
        PRIME_ASSERT(_readHasBeenCalled);
        return _filename.c_str();
    }

    WindowsDirectoryReader& operator=(const WindowsDirectoryReader& copy);

private:
    static bool isDotFilename(LPCTSTR path);
    static bool isDotDotFilename(LPCTSTR path);
    static LPCTSTR findFilename(LPCTSTR path);

    HANDLE _handle;
    WIN32_FIND_DATA _data;
    bool _first;
    bool _readHasBeenCalled;
    std::string _filename;
};

}

#endif
