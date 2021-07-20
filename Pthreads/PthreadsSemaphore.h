// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PTHREADS_PTHREADSSEMAPHORE_H
#define PRIME_PTHREADS_PTHREADSSEMAPHORE_H

#include "../Config.h"

// Darwin only implements POSIX semaphores for named semaphores, so EmulatedSemaphore is used instead.
#if !defined(PRIME_OS_OSX)

#include "../Log.h"
#include "../ScopedLock.h"
#include <semaphore.h>

#define PRIME_OS_HAS_PTHREADSSEMAPHORE

namespace Prime {

/// A semaphore implemented using the standard POSIX semaphore APIs.
class PRIME_PUBLIC PthreadsSemaphore {
public:
    typedef Prime::ScopedLock<PthreadsSemaphore> ScopedLock;

    /// Construct and specify the initial count of the semaphore.
    PthreadsSemaphore()
        : _initialised(false)
    {
    }

    PthreadsSemaphore(int initialCount, Log* log, const char* debugName = NULL, int maximumCount = INT_MAX)
        : _initialised(false)
    {
        PRIME_EXPECT(init(initialCount, log, debugName, maximumCount));
    }

    ~PthreadsSemaphore() { close(); }

    bool init(int initialCount, Log* log, const char* debugName = NULL, int maximumCount = INT_MAX);

    void close();

    bool isInitialised() const { return _initialised; }

    /// Lock the semaphore, waiting for as long as necessary.
    void lock();

    /// Lock the semaphore n times, waiting for as long as necessary.
    void lock(int n);

    /// Unlock the semaphore.
    void unlock() { post(1); }

    /// Increment the count by the specified amount.
    void post(int increment = 1);

    /// Try to lock the semaphore. Returns true if the lock was obtained.
    bool tryLock();

    /// Try to lock the semaphore within the specified number of milliseconds. Returns true if the semaphore was
    /// locked.
    bool tryLock(int milliseconds);

private:
    sem_t _sem;
    bool _initialised;

    PRIME_UNCOPYABLE(PthreadsSemaphore);
};
}

#endif

#endif
