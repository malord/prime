// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PTHREADS_PTHREADSMUTEX_H
#define PRIME_PTHREADS_PTHREADSMUTEX_H

#include "../Log.h"
#include "../ScopedLock.h"
#include <pthread.h>

namespace Prime {

/// Wraps a pthreads mutex.
class PRIME_PUBLIC PthreadsMutex {
public:
    typedef Prime::ScopedLock<PthreadsMutex> ScopedLock;

    class Attributes {
    private:
        bool _recursive;

    public:
        Attributes()
            : _recursive(false)
        {
        }

        Attributes& setRecursive(bool value = true)
        {
            _recursive = value;
            return *this;
        }
        bool getRecursive() const { return _recursive; }
    };

    PthreadsMutex()
        : _initialised(false)
    {
    }

    explicit PthreadsMutex(Log* log, const char* debugName = NULL, const Attributes& attributes = Attributes())
        : _initialised(false)
    {
        PRIME_EXPECT(init(log, debugName, attributes));
    }

    ~PthreadsMutex() { close(); }

    bool init(Log* log, const char* debugName = NULL, const Attributes& attributes = Attributes());

    void close();

    bool isInitialised() const { return _initialised; }

    /// Lock the mutex, waiting for as long as necessary.
    void lock()
    {
        PRIME_ASSERT(_initialised);
        PRIME_EXPECT(pthread_mutex_lock(&_mutex) == 0);
    }

    /// Unlock the mutex.
    void unlock()
    {
        PRIME_ASSERT(_initialised);
        PRIME_EXPECT(pthread_mutex_unlock(&_mutex) == 0);
    }

    /// Try to lock the mutex. Returns true if the lock was obtained.
    bool tryLock()
    {
        PRIME_ASSERT(_initialised);
        return pthread_mutex_trylock(&_mutex) == 0;
    }

private:
    friend class PthreadsCondition;

    /// Direct access to the pthread mutex for PthreadsCondition.
    pthread_mutex_t* getPthreadsMutex() const { return &_mutex; }

    mutable pthread_mutex_t _mutex;
    bool _initialised;

    PRIME_UNCOPYABLE(PthreadsMutex);
};
}

#endif
