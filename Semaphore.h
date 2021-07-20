// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SEMAPHORE_H
#define PRIME_SEMAPHORE_H

#include "Lock.h"

namespace Prime {

class Log;

/// A do-nothing semaphore for when threading is disabled.
class NullSemaphore {
public:
    typedef Prime::ScopedLock<NullSemaphore> ScopedLock;

    /// Construct and specify the initial count of the semaphore.
    explicit NullSemaphore()
        : _count(0)
    {
    }

    bool init(int initialCount, Log* log, const char* debugName = NULL, int maximumCount = INT_MAX)
    {
        (void)log;
        (void)debugName;
        (void)maximumCount;
        _count = initialCount;
        return true;
    }

    void close() { }

    bool isInitialised() const { return true; }

    /// Lock the semaphore, waiting for as long as necessary.
    void lock() { --_count; }

    /// Lock the semaphore n times, waiting for as long as necessary.
    void lock(int n) { _count -= n; }

    /// Unlock the semaphore.
    void unlock() { post(1); }

    /// Unlock the semaphore, increasing the count by the specified amount.
    void post(int increment = 1) { _count += increment; }

    /// Try to lock the semaphore, but return instantly if another thread has locked it. Returns true if the lock
    /// was obtained.
    bool tryLock()
    {
        if (!_count) {
            return false;
        }

        --_count;
        return true;
    }

    /// Try to lock the semaphore, but return instantly if another thread has locked it. Returns true if the lock
    /// was obtained.
    bool tryLock(int milliseconds)
    {
        (void)milliseconds;

        if (!_count) {
            return false;
        }

        --_count;
        return true;
    }

private:
    int _count;

    PRIME_UNCOPYABLE(NullSemaphore);
};
}

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsSemaphore.h"

namespace Prime {
typedef WindowsSemaphore Semaphore;
typedef WindowsSemaphore TrySemaphore;
typedef WindowsSemaphore TimedSemaphore;
}

#elif defined(PRIME_OS_UNIX)

#if !defined(PRIME_NO_GCD)
#include "OSX/GCDConfig.h"
#endif

/// Use libdispatch semaphores in preferences to POSIX semaphores
#if !defined(PRIME_NO_GCD)

#include "OSX/GCDSemaphore.h"

namespace Prime {
typedef GCDSemaphore Semaphore;
typedef GCDSemaphore TrySemaphore;
typedef GCDSemaphore TimedSemaphore;
}

// OS X doesn't have anonymous POSIX semaphores
#elif defined(PRIME_OS_OSX)

#include "Emulated/EmulatedSemaphore.h"

namespace Prime {
typedef EmulatedSemaphore Semaphore;
typedef EmulatedSemaphore TrySemaphore;
typedef EmulatedSemaphore TimedSemaphore;
}

#else

#include "Pthreads/PthreadsSemaphore.h"
#if !defined(PRIME_OS_LINUX)
#include "Emulated/EmulatedSemaphore.h"
#endif

namespace Prime {
typedef PthreadsSemaphore Semaphore;
typedef PthreadsSemaphore TrySemaphore;

#ifdef PRIME_OS_LINUX
typedef PthreadsSemaphore TimedSemaphore;
#else
typedef EmulatedSemaphore TimedSemaphore;
#endif
}

#endif

#else

namespace Prime {
typedef NullSemaphore Semaphore;
typedef NullSemaphore TrySemaphore;
typedef NullSemaphore TimedSemaphore;
}

#endif

namespace Prime {

/// Implement the Lock interface using a Semaphore.
template <typename Semaphore>
class SemaphoreLock : public Lock {
public:
    bool init(int initialCount, Log* log, const char* debugName = NULL, int maximumCount = INT_MAX)
    {
        return _semaphore.init(initialCount, log, debugName, maximumCount);
    }

    virtual void lock() { _semaphore.lock(); }

    virtual void unlock() { _semaphore.unlock(); }

private:
    Semaphore _semaphore;
};
}

#endif
