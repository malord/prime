// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_GCD_GCDSEMAPHORE_H
#define PRIME_GCD_GCDSEMAPHORE_H

#include "GCDConfig.h"

#ifndef PRIME_NO_GCD

#include "../Log.h"
#include "../ScopedLock.h"

namespace Prime {

/// Prime
class PRIME_PUBLIC GCDSemaphore {
public:
    typedef Prime::ScopedLock<GCDSemaphore> ScopedLock;

    GCDSemaphore()
        : _semaphore(NULL)
    {
    }

    GCDSemaphore(long initialCount, Log* log, const char* debugName = NULL, long /*maximumCount*/ = LONG_MAX)
        : _semaphore(NULL)
    {
        PRIME_EXPECT(init(initialCount, log, debugName));
    }

    ~GCDSemaphore() { close(); }

    bool init(long initialCount, Log* log, const char* debugName = NULL, long /*maximumCount*/ = LONG_MAX);

    void close();

    bool isInitialised() const { return _semaphore != NULL; }

    /// Lock the semaphore, waiting for as long as necessary.
    void lock()
    {
        PRIME_ASSERT(isInitialised());
        dispatch_semaphore_wait(_semaphore, DISPATCH_TIME_FOREVER);
    }

    /// Lock the semaphore n times, waiting for as long as necessary.
    void lock(long n);

    /// Unlock the semaphore.
    void unlock()
    {
        PRIME_ASSERT(isInitialised());
        dispatch_semaphore_signal(_semaphore);
    }

    /// Unlock the semaphore, increasing the count by the specified amount.
    void post(long increment = 1);

    /// Try to lock the semaphore, but return instantly if another thread has locked it. Returns true if the lock
    /// was obtained.
    bool tryLock()
    {
        PRIME_ASSERT(isInitialised());
        return dispatch_semaphore_wait(_semaphore, DISPATCH_TIME_NOW) == 0;
    }

    /// Try to lock the semaphore within the specified number of milliseconds. Returns true if the semaphore was
    /// locked.
    bool tryLock(int milliseconds);

private:
    dispatch_semaphore_t _semaphore;

    PRIME_UNCOPYABLE(GCDSemaphore);
};
}

#endif // PRIME_NO_GCD

#endif
