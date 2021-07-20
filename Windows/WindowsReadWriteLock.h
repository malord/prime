// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSREADWRITELOCK_H
#define PRIME_WINDOWS_WINDOWSREADWRITELOCK_H

#include "../Log.h"
#include "../ScopedLock.h"
#include "WindowsConfig.h"

namespace Prime {

/// A read/write lock for Windows.
class PRIME_PUBLIC WindowsReadWriteLock {
public:
    typedef Prime::ScopedReadLock<WindowsReadWriteLock> ScopedReadLock;
    typedef Prime::ScopedWriteLock<WindowsReadWriteLock> ScopedWriteLock;

    WindowsReadWriteLock()
        : _writeMutex(NULL)
        , _readEvent(NULL)
    {
    }

    explicit WindowsReadWriteLock(Log* log, const char* debugName = NULL)
        : _writeMutex(NULL)
        , _readEvent(NULL)
    {
        PRIME_EXPECT(init(log, debugName));
    }

    ~WindowsReadWriteLock() { close(); }

    bool init(Log* log, const char* debugName = NULL);

    void close();

    bool isInitialised() const { return _writeMutex != NULL; }

    /// Lock for reading.
    void lockRead()
    {
        PRIME_ASSERT(isInitialised());
        tryLockRead(INFINITE);
    }

    /// Try to lock for reading. Returns true if the lock was obtained.
    bool tryLockRead()
    {
        PRIME_ASSERT(isInitialised());
        return tryLockRead(0);
    }

    /// Try to lock for reading within the specified number of milliseconds. Returns true if the lock was obtained.
    bool tryLockRead(int milliseconds);

    /// Unlock the read lock.
    void unlockRead();

    /// Lock for writing.
    void lockWrite()
    {
        PRIME_ASSERT(isInitialised());
        tryLockWrite(INFINITE);
    }

    /// Try to lock for writing. Returns true if the lock was obtained.
    bool tryLockWrite()
    {
        PRIME_ASSERT(isInitialised());
        return tryLockWrite(0);
    }

    /// Try to lock for writing within the specified number of milliseconds. Returns true if the lock was obtained.
    bool tryLockWrite(int milliseconds);

    /// Unlock the write lock.
    void unlockWrite();

private:
#if WINVER >= 0x0500
    volatile LONG _readerCount;
#else
    LONG _readerCount;
#endif
    HANDLE _writeMutex;
    HANDLE _readEvent;

    PRIME_UNCOPYABLE(WindowsReadWriteLock);
};

}

#endif
