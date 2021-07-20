// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSFILEPROPERTIES_H
#define PRIME_WINDOWS_WINDOWSFILEPROPERTIES_H

#include "WindowsDirectoryReader.h"

namespace Prime {

class PRIME_PUBLIC WindowsFileProperties {
public:
    WindowsFileProperties();

    ~WindowsFileProperties();

    bool isInitialised() const { return _dir.isInitialised(); }

    /// Retrieve the find data for the specified path then immediately close the search.
    bool read(const char* path, Log* log);

    /// Retrieve the find data for the spcecified path then immediately close the path.
    bool readTChar(LPCTSTR path, Log* log);

    bool readLink(const char* path, Log* log)
    {
        // TODO: support junctions or Vista symlinks?
        return read(path, log);
    }

    UnixTime getCreationTime() const { return _dir.getCreationTime(); }

    UnixTime getLastAccessTime() const { return _dir.getLastAccessTime(); }

    UnixTime getLastWriteTime() const { return _dir.getLastWriteTime(); }

    UnixTime getModificationTime() const { return getLastWriteTime(); }

    int64_t getSize() const { return _dir.getSize(); }

    DWORD getWindowsAttributes() const { return _dir.getWindowsAttributes(); }

    bool isDirectory() const { return _dir.isDirectory(); }

#ifdef FILE_ATTRIBUTE_DEVICE
    bool isDevice() const
    {
        return _dir.isDevice();
    }
#endif

    bool isHidden() const
    {
        return _dir.isHidden();
    }

    bool isReadOnly() const { return _dir.isReadOnly(); }

    bool isSystem() const { return _dir.isSystem(); }

    bool isLink() const { return _dir.isLink(); }

    bool isFile() const { return _dir.isFile(); }

    /// Read just the file times of the specified path.
    bool getTimes(const char* path, Log* log)
    {
        // FindFirstFile is just as easy as CreateFile() followed by GetFileTimes()...
        return read(path, log);
    }

    /// Apply the current file times to the specified path.
    bool applyTimes(const char* path, Log* log) const;

    void setCreationTime(const UnixTime& unixTime);

    void setLastAccessTime(const UnixTime& unixTime);

    void setModificationTime(const UnixTime& unixTime);

    /// Alternate name for compatibility with Windows.
    void setLastWriteTime(const UnixTime& unixTime) { setModificationTime(unixTime); }

    /// Imbue the current attributes on another file.
    bool applyWindowsAttributes(const char* path, Log* log) const;

    /// Imbue the current attributes on another file.
    bool applyMode(const char* path, Log* log) const
    {
        return applyWindowsAttributes(path, log);
    }

private:
    WindowsDirectoryReader _dir;
};
}

#endif
