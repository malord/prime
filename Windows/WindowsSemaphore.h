// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSSEMAPHORE_H
#define PRIME_WINDOWS_WINDOWSSEMAPHORE_H

#include "../Log.h"
#include "../ScopedLock.h"
#include "WindowsConfig.h"

namespace Prime {

class PRIME_PUBLIC WindowsSemaphore {
public:
    typedef Prime::ScopedLock<WindowsSemaphore> ScopedLock;

    WindowsSemaphore()
        : _semaphore(NULL)
    {
    }

    WindowsSemaphore(LONG initialCount, Log* log, const char* debugName = NULL, LONG maximumCount = LONG_MAX)
        : _semaphore(NULL)
    {
        PRIME_EXPECT(init(initialCount, log, debugName, maximumCount));
    }

    ~WindowsSemaphore() { close(); }

    bool init(LONG initialCount, Log* log, const char* debugName = NULL, LONG maximumCount = LONG_MAX);

    void close();

    bool isInitialised() const { return _semaphore != NULL; }

    /// Lock the semaphore, waiting for as long as necessary.
    void lock()
    {
        PRIME_ASSERT(isInitialised());
        WaitForSingleObject(_semaphore, INFINITE);
    }

    /// Lock the semaphore n times, waiting for as long as necessary.
    void lock(LONG n);

    /// Unlock the semaphore.
    void unlock()
    {
        PRIME_ASSERT(isInitialised());
        post(1);
    }

    /// Unlock the semaphore, increasing the count by the specified amount.
    void post(LONG increment = 1)
    {
        PRIME_ASSERT(isInitialised());
        ReleaseSemaphore(_semaphore, increment, 0);
    }

    /// Try to lock the semaphore, but return instantly if another thread has locked it. Returns true if the lock
    /// was obtained.
    bool tryLock()
    {
        PRIME_ASSERT(isInitialised());
        return WaitForSingleObject(_semaphore, 0) == WAIT_OBJECT_0;
    }

    /// Try to lock the semaphore within the specified number of milliseconds. Returns true if the semaphore was
    /// locked.
    bool tryLock(int milliseconds)
    {
        PRIME_ASSERT(isInitialised());
        return WaitForSingleObject(_semaphore, milliseconds < 0 ? INFINITE : (DWORD)milliseconds) == WAIT_OBJECT_0;
    }

private:
    HANDLE _semaphore;

    PRIME_UNCOPYABLE(WindowsSemaphore);
};

}

#endif
