// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PTHREADS_PTHREADSREADWRITELOCK_H
#define PRIME_PTHREADS_PTHREADSREADWRITELOCK_H

#include "../Log.h"
#include "../ScopedLock.h"
#include <pthread.h>

namespace Prime {

/// Wraps a pthreads read/write lock.
class PRIME_PUBLIC PthreadsReadWriteLock {
public:
    typedef Prime::ScopedReadLock<PthreadsReadWriteLock> ScopedReadLock;
    typedef Prime::ScopedWriteLock<PthreadsReadWriteLock> ScopedWriteLock;

    PthreadsReadWriteLock()
        : _initialised(false)
    {
    }

    explicit PthreadsReadWriteLock(Log* log, const char* debugName = NULL)
        : _initialised(false)
    {
        PRIME_EXPECT(init(log, debugName));
    }

    ~PthreadsReadWriteLock() { close(); }

    bool init(Log* log, const char* debugName = NULL);

    void close();

    bool isInitialised() const { return _initialised; }

    /// Lock for reading.
    void lockRead()
    {
        PRIME_ASSERT(_initialised);
        PRIME_EXPECT(pthread_rwlock_rdlock(&_rwlock) == 0);
    }

    /// Try to lock for reading. Returns true if the lock was obtained.
    bool tryLockRead()
    {
        PRIME_ASSERT(_initialised);
        return pthread_rwlock_tryrdlock(&_rwlock) == 0;
    }

    /// Unlock the read lock.
    void unlockRead()
    {
        PRIME_ASSERT(_initialised);
        PRIME_EXPECT(pthread_rwlock_unlock(&_rwlock) == 0);
    }

    /// Lock for writing.
    void lockWrite()
    {
        PRIME_ASSERT(_initialised);
        PRIME_EXPECT(pthread_rwlock_wrlock(&_rwlock) == 0);
    }

    /// Try to lock for writing. Returns true if the lock was obtained.
    bool tryLockWrite()
    {
        PRIME_ASSERT(_initialised);
        return pthread_rwlock_trywrlock(&_rwlock) == 0;
    }

    /// Unlock the write lock.
    void unlockWrite()
    {
        PRIME_ASSERT(_initialised);
        PRIME_EXPECT(pthread_rwlock_unlock(&_rwlock) == 0);
    }

private:
    pthread_rwlock_t _rwlock;
    bool _initialised;

    PRIME_UNCOPYABLE(PthreadsReadWriteLock);
};

}

#endif
