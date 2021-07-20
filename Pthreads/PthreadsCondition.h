// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PTHREADS_PTHREADSCONDITION_H
#define PRIME_PTHREADS_PTHREADSCONDITION_H

#include "../Log.h"
#include <pthread.h>

namespace Prime {

/// Wraps a pthreads condition variable.
class PRIME_PUBLIC PthreadsCondition {
public:
    PthreadsCondition()
        : _initialised(false)
    {
    }

    explicit PthreadsCondition(Log* log, const char* debugName = NULL)
        : _initialised(false)
    {
        PRIME_EXPECT(init(log, debugName));
    }

    template <typename Mutex>
    PthreadsCondition(Mutex* mutex, Log* log, const char* debugName = NULL)
        : _initialised(false)
    {
        PRIME_EXPECT(init(mutex, log, debugName));
    }

    ~PthreadsCondition() { close(); }

    /// On some platforms, the Mutex must be specified at the time the condition variable is created. Under
    /// pthreads, ignore the mutex.
    template <typename Mutex>
    bool init(Mutex*, Log* log, const char* debugName = NULL)
    {
        return init(log, debugName);
    }

    bool init(Log* log, const char* debugName = NULL);

    void close();

    bool isInitialised() const { return _initialised; }

    /// Notify (wake) a single waiter.
    void wakeOne()
    {
        PRIME_ASSERT(_initialised);
        PRIME_EXPECT(pthread_cond_signal(&_condition) == 0);
    }

    /// Notify (wake) all waiters.
    void wakeAll()
    {
        PRIME_ASSERT(_initialised);
        PRIME_EXPECT(pthread_cond_broadcast(&_condition) == 0);
    }

    /// Wait on this condition and acquire the specified lock (which must be a pthreads mutex) once the condition
    /// is activated.
    template <typename MutexScopedLock>
    void wait(MutexScopedLock& lock)
    {
        PRIME_ASSERT(_initialised);
        pthreadsWait(lock.getLockable()->getPthreadsMutex());
    }

    /// Wait on this condition and acquire the specified lock (which must be a pthreads mutex) once the condition
    /// is activated. Returns true if the condition was triggered and the lock obtained.
    template <typename MutexScopedLock>
    bool timedWait(MutexScopedLock& lock, int milliseconds)
    {
        PRIME_ASSERT(_initialised);
        return pthreadsTimedWait(lock.getLockable()->getPthreadsMutex(), milliseconds);
    }

private:
    void pthreadsWait(pthread_mutex_t* mutex)
    {
        PRIME_ASSERT(_initialised);
        PRIME_EXPECT(pthread_cond_wait(&_condition, mutex) == 0);
    }

    bool pthreadsTimedWait(pthread_mutex_t* mutex, int milliseconds);

    pthread_cond_t _condition;
    bool _initialised;

    PRIME_UNCOPYABLE(PthreadsCondition);
};
}

#endif
