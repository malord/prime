// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXFILEPROPERTIES_H
#define PRIME_UNIX_UNIXFILEPROPERTIES_H

#include "../Log.h"
#include "../UnixTime.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace Prime {

/// Wrapper around the stat() and lstat() API.
class UnixFileProperties {
public:
    UnixFileProperties()
        : _initialised(false)
    {
    }

    bool isInitialised() const { return _initialised; }

    /// Calls stat() on the supplied path.
    bool read(const char* path, Log* log);

    /// Calls lstat() on the supplied path.
    bool readLink(const char* path, Log* log);

    /// Calls fstat() on the supplied file descriptor.
    bool readHandle(int handle, Log* log);

    UnixTime getStatusChangeTime() const;

    UnixTime getLastAccessTime() const;

    UnixTime getModificationTime() const;

    /// Alternate name for compatibility with Windows.
    UnixTime getLastWriteTime() const { return getModificationTime(); }

    int64_t getSize() const
    {
        PRIME_ASSERT(_initialised);
        return _stat.st_size;
    }

    mode_t getMode() const
    {
        PRIME_ASSERT(_initialised);
        return _stat.st_mode;
    }

    bool isDirectory() const
    {
        PRIME_ASSERT(_initialised);
        return S_ISDIR(_stat.st_mode) != 0;
    }

    bool isDevice() const
    {
        PRIME_ASSERT(_initialised);
        return S_ISBLK(_stat.st_mode) || S_ISCHR(_stat.st_mode);
    }

    bool isHidden() const
    {
        PRIME_ASSERT(_initialised);
        return _hidden;
    }

    /// For compatibility with Windows.
    bool isReadOnly() const
    {
        PRIME_ASSERT(_initialised);
        return false;
    }

    /// For compatibility with Windows.
    bool isSystem() const
    {
        PRIME_ASSERT(_initialised);
        return false;
    }

    bool isLink() const
    {
        PRIME_ASSERT(_initialised);
        return S_ISLNK(_stat.st_mode);
    }

    bool isFile() const
    {
        PRIME_ASSERT(_initialised);
        return S_ISREG(_stat.st_mode);
    }

    bool isFIFO() const
    {
        PRIME_ASSERT(_initialised);
        return S_ISFIFO(_stat.st_mode);
    }

    bool isSocket() const
    {
        PRIME_ASSERT(_initialised);
        return S_ISSOCK(_stat.st_mode);
    }

    /// Read just the file times of the specified path.
    bool getTimes(const char* path, Log* log)
    {
        return read(path, log);
    }

    /// Apply the current file times to the specified path.
    bool applyTimes(const char* path, Log* log) const;

    void setStatusChangeTime(const UnixTime& unixTime);

    void setLastAccessTime(const UnixTime& unixTime);

    void setModificationTime(const UnixTime& unixTime);

    /// Alternate name for compatibility with Windows.
    void setLastWriteTime(const UnixTime& unixTime) { return setModificationTime(unixTime); }

    /// Set the mode of the supplied path to the mode from the read() call.
    bool applyMode(const char* path, Log* log) const;

    const struct stat& getStat() const
    {
        PRIME_ASSERT(_initialised);
        return _stat;
    }

    const struct stat* operator->() const
    {
        PRIME_ASSERT(_initialised);
        return &_stat;
    }

private:
    struct stat _stat;
    bool _initialised;
    bool _hidden;
};
}

#endif
