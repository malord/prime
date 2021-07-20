// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MUTEX_H
#define PRIME_MUTEX_H

#include "Lock.h"

namespace Prime {

class Log;

/// A no-op mutex.
class NullMutex {
public:
    typedef Prime::ScopedLock<NullMutex> ScopedLock;

    NullMutex() { }

    bool init(Log* log, const char* debugName = NULL)
    {
        (void)log;
        (void)debugName;
        return true;
    }

    void close() { }

    bool isInitialised() const { return true; }

    /// Lock the mutex, waiting for as long as necessary.
    void lock() { }

    /// Unlock the mutex.
    void unlock() { }

    /// Try to lock the mutex. Returns true if it was locked.
    bool tryLock() { return true; }

    PRIME_UNCOPYABLE(NullMutex);
};
}

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsCriticalSection.h"
#include "Windows/WindowsMutex.h"
#include "Windows/WindowsNonRecursiveMutex.h"

namespace Prime {
typedef WindowsCriticalSection Mutex;
typedef WindowsMutex TryMutex;
typedef WindowsMutex TimedMutex;

typedef WindowsCriticalSection RecursiveMutex;
typedef WindowsMutex RecursiveTryMutex;
typedef WindowsMutex RecursiveTimedMutex;

typedef WindowsNonRecursiveMutex NonRecursiveMutex;
typedef WindowsNonRecursiveMutex NonRecursiveTryMutex;
typedef WindowsNonRecursiveMutex NonRecursiveTimedMutex;
}

#elif defined(PRIME_OS_UNIX)

#include "Pthreads/PthreadsMutex.h"
#include "Pthreads/PthreadsNonRecursiveMutex.h"
#include "Pthreads/PthreadsRecursiveMutex.h"
#include "Pthreads/PthreadsRecursiveTimedMutex.h"

namespace Prime {
typedef PthreadsMutex Mutex;
typedef PthreadsMutex TryMutex;
typedef PthreadsRecursiveTimedMutex TimedMutex;

typedef PthreadsRecursiveMutex RecursiveMutex;
typedef PthreadsRecursiveMutex RecursiveTryMutex;
typedef PthreadsRecursiveTimedMutex RecursiveTimedMutex;

typedef PthreadsNonRecursiveMutex NonRecursiveMutex;
typedef PthreadsNonRecursiveMutex NonRecursiveTryMutex;
// FUTURE: NonRecursiveTimedMutex;
}

#else

namespace Prime {
typedef NullMutex Mutex;
typedef NullMutex TryMutex;
typedef NullMutex TimedMutex;

typedef NullMutex RecursiveMutex;
typedef NullMutex RecursiveTryMutex;
typedef NullMutex RecursiveTimedMutex;

typedef NullMutex NonRecursiveMutex;
typedef NullMutex NonRecursiveTryMutex;
typedef NullMutex NonRecursiveTimedMutex;
}

#endif

namespace Prime {

/// Implement the Lock interface using a Mutex.
template <typename Mutex>
class MutexLock : public Lock {
public:
    bool init(Log* log, const char* debugName = NULL)
    {
        return _mutex.init(log, debugName);
    }

    virtual void lock() { _mutex.lock(); }

    virtual void unlock() { _mutex.unlock(); }

private:
    Mutex _mutex;
};
}

#endif
