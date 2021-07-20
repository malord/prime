// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PTHREADS_PTHREADSRECURSIVETIMEDMUTEX_H
#define PRIME_PTHREADS_PTHREADSRECURSIVETIMEDMUTEX_H

#include "../Log.h"
#include "../ScopedLock.h"
#include <pthread.h>

namespace Prime {

/// A mutex capable of recursion and timed locking.
class PRIME_PUBLIC PthreadsRecursiveTimedMutex {
public:
    typedef Prime::ScopedLock<PthreadsRecursiveTimedMutex> ScopedLock;

    PthreadsRecursiveTimedMutex()
        : _initialised(false)
    {
    }

    explicit PthreadsRecursiveTimedMutex(Log* log, const char* debugName = NULL)
        : _initialised(false)
    {
        PRIME_EXPECT(init(log, debugName));
    }

    ~PthreadsRecursiveTimedMutex() { close(); }

    bool init(Log* log, const char* debugName = NULL);

    void close();

    bool isInitialised() const { return _initialised; }

    /// Lock the mutex, waiting for as long as necessary.
    void lock();

    /// Unlock the mutex.
    void unlock();

    /// Try to lock the mutex. Returns true if the lock was obtained.
    bool tryLock();

    /// Try to lock the mutex within the specified number of milliseconds. Returns true if the lock was
    /// obtained.
    bool tryLock(int milliseconds);

private:
    friend class PthreadsCondition;

    /// Direct access to the pthread mutex.
    pthread_mutex_t* getPthreadsMutex() const
    {
        PRIME_ASSERT(_initialised);
        return &_mutex;
    }

    mutable pthread_mutex_t _mutex;
    pthread_cond_t _unlocked;
    volatile int _reentered;
    volatile bool _locked;
    volatile pthread_t _lockedByThread;
    bool _initialised;

    PRIME_UNCOPYABLE(PthreadsRecursiveTimedMutex);
};

}

#endif
