// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_EMULATED_EMULATEDSEMAPHORE_H
#define PRIME_EMULATED_EMULATEDSEMAPHORE_H

#include "../Condition.h"
#include "../Mutex.h"

namespace Prime {

/// A semaphore implemented using a mutex and a condition variable. Has a timed lock facility, which some native
/// semaphores lack.
class PRIME_PUBLIC EmulatedSemaphore {
public:
    typedef Prime::ScopedLock<EmulatedSemaphore> ScopedLock;

    /// Construct and specify the initial count of the semaphore.
    EmulatedSemaphore() { }

    EmulatedSemaphore(int initialCount, Log* log, const char* debugName = NULL, int maximumCount = INT_MAX)
    {
        PRIME_EXPECT(init(initialCount, log, debugName, maximumCount));
    }

    ~EmulatedSemaphore() { close(); }

    bool init(int initialCount, Log* log, const char* debugName = NULL, int = INT_MAX);

    void close();

    bool isInitialised() const { return _nonzero.isInitialised(); }

    /// Lock the semaphore n times, waiting for as long as necessary.
    void lock(int n = 1);

    /// Unlock the semaphore.
    void unlock() { post(1); }

    /// Increment the count by the specified amount. Returns true if any threads were waiting.
    bool post(int increment = 1);

    /// Try to lock the semaphore. Returns true if the lock was obtained.
    bool tryLock();

    /// Try to lock the semaphore within the specified number of milliseconds. Returns true if the semaphore was
    /// locked.
    bool tryLock(int milliseconds);

private:
    volatile int _count;
    volatile int _numberWaiting;
    NonRecursiveMutex _mutex;
    Condition _nonzero;

    PRIME_UNCOPYABLE(EmulatedSemaphore);
};
}

#endif
