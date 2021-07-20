// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSMUTEX_H
#define PRIME_WINDOWS_WINDOWSMUTEX_H

#include "../Log.h"
#include "../ScopedLock.h"
#include "WindowsConfig.h"

namespace Prime {

/// Wrapper around a Windows' mutex (which is a recursive mutex).
class PRIME_PUBLIC WindowsMutex {
public:
    typedef Prime::ScopedLock<WindowsMutex> ScopedLock;

    /// Construct, optionally specifying whether the mutex should be immediately owned.
    WindowsMutex()
        : _mutex(NULL)
    {
    }

    explicit WindowsMutex(Log* log, const char* debugName = NULL)
        : _mutex(NULL)
    {
        PRIME_EXPECT(init(log, debugName));
    }

    ~WindowsMutex() { close(); }

    bool init(Log* log, const char* debugName = NULL);

    void close()
    {
        if (_mutex) {
            CloseHandle(_mutex);
            _mutex = 0;
        }
    }

    bool isInitialised() const { return _mutex != 0; }

    /// Lock the mutex, waiting for as long as necessary.
    void lock()
    {
        PRIME_ASSERT(isInitialised());
        WaitForSingleObject(_mutex, INFINITE);
    }

    /// Unlock the mutex.
    void unlock()
    {
        PRIME_ASSERT(isInitialised());
        ReleaseMutex(_mutex);
    }

    /// Try to lock the mutex, but return instantly if another thread has locked it. Returns true if the lock was
    /// obtained.
    bool tryLock()
    {
        PRIME_ASSERT(isInitialised());
        return WaitForSingleObject(_mutex, 0) == WAIT_OBJECT_0;
    }

    /// Try to lock the mutex within the specified number of milliseconds. Returns true if the mutex was locked.
    bool tryLock(int milliseconds)
    {
        PRIME_ASSERT(isInitialised());
        return WaitForSingleObject(_mutex, milliseconds < 0 ? INFINITE : (DWORD)milliseconds) == WAIT_OBJECT_0;
    }

private:
    HANDLE _mutex;

    PRIME_UNCOPYABLE(WindowsMutex);
};

}

#endif
