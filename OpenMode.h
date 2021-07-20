// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OPENMODE_H
#define PRIME_OPENMODE_H

#include "Config.h"

namespace Prime {

/// How a file is opened.
class OpenMode {
public:
    OpenMode()
        : _read(false)
        , _write(false)
        , _create(false)
        , _truncate(false)
        , _doNotOverwrite(false)
        , _childInherit(false)
        , _doNotCache(false)
        , _bufferSequential(false)
        , _bufferRandom(false)
        , _append(false)
        , _useUnixPermissions(false)
        , _unixPermssions(0)
        , _syncOnClose(false)
    {
    }

    OpenMode& setRead(bool value = true)
    {
        _read = value;
        return *this;
    }
    bool getRead() const { return _read; }

    OpenMode& setWrite(bool value = true)
    {
        _write = value;
        return *this;
    }
    bool getWrite() const { return _write; }

    OpenMode& setReadWrite() { return setRead().setWrite(); }
    bool getReadWrite() const { return _read && _write; }

    OpenMode& setCreate(bool value = true)
    {
        _create = value;
        return *this;
    }
    bool getCreate() const { return _create; }

    OpenMode& setTruncate(bool value = true)
    {
        _truncate = value;
        return *this;
    }
    bool getTruncate() const { return _truncate; }

    OpenMode& setDoNotOverwrite(bool value = true)
    {
        _doNotOverwrite = value;
        return *this;
    }
    bool getDoNotOverwrite() const { return _doNotOverwrite; }

    /// By default, platforms that support it will prevent file handles being inherited by child processes.
    /// Use this flag if you need a child process to inherit a file handle.
    OpenMode& setChildProcessInherit(bool value = true)
    {
        _childInherit = value;
        return *this;
    }
    bool getChildProcessInherit() const { return _childInherit; }

    /// In an application that implements caching, specify that the cache should not retain the file in memory.
    /// Use this if you know the file will not be loaded again.
    OpenMode& setDoNotCache(bool value = true)
    {
        _doNotCache = value;
        return *this;
    }
    bool getDoNotCache() const { return _doNotCache; }

    /// A hint to any buffering system that the file will be read/written sequentially.
    OpenMode& setBufferSequential(bool value = true)
    {
        _bufferSequential = value;
        return *this;
    }
    bool getBufferSequential() const { return _bufferSequential; }

    /// A hint to any buffering system that the file will be read/written at random offsets.
    OpenMode& setBufferRandomAccess(bool value = true)
    {
        _bufferRandom = value;
        return *this;
    }
    bool getBufferRandomAccess() const { return _bufferRandom; }

    OpenMode& setAppend(bool value = true)
    {
        _append = value;
        return *this;
    }
    bool getAppend() const { return _append; }

    OpenMode& setUseUnixPermissions(bool value = true)
    {
        _useUnixPermissions = value;
        return *this;
    }
    bool getUseUnixPermissions() const { return _useUnixPermissions; }

    OpenMode& setUnixPermissions(unsigned int value)
    {
        _unixPermssions = value;
        return *this;
    }
    unsigned int getUnixPermissions() const { return _unixPermssions; }

    OpenMode& setUnixTempPermissions() { return setUseUnixPermissions().setUnixPermissions(0600); }

    OpenMode& setSyncOnClose(bool value = true)
    {
        _syncOnClose = value;
        return *this;
    }
    bool getSyncOnClose() const { return _syncOnClose; }

    bool isWriteAccessRequired() const { return _write || _create || _truncate; }

    OpenMode& setOverwrite() { return setWrite().setCreate().setTruncate(); }

private:
    bool _read;
    bool _write;
    bool _create;
    bool _truncate;
    bool _doNotOverwrite;
    bool _childInherit;
    bool _doNotCache;
    bool _bufferSequential;
    bool _bufferRandom;
    bool _append;
    bool _useUnixPermissions;
    unsigned int _unixPermssions;
    bool _syncOnClose;
};
}

#endif
