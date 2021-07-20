// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_READWRITELOCK_H
#define PRIME_READWRITELOCK_H

#include "ScopedLock.h"

namespace Prime {

class Log;

/// A no-op read/write lock used when threading is disabled.
class NullReadWriteLock {
public:
    typedef Prime::ScopedReadLock<NullReadWriteLock> ScopedReadLock;
    typedef Prime::ScopedWriteLock<NullReadWriteLock> ScopedWriteLock;

    NullReadWriteLock() { }

    bool init(Log* log, const char* debugName = NULL)
    {
        (void)log;
        (void)debugName;
        return true;
    }

    void close() { }

    bool isInitialised() const { return true; }

    /// Lock for reading.
    void lockRead() { }

    /// Try to lock for reading. Returns true if the lock was obtained.
    bool tryLockRead() { return true; }

    /// Unlock the read lock.
    void unlockRead() { }

    /// Lock for writing.
    void lockWrite() { }

    /// Try to lock for writing. Returns true if the lock was obtained.
    bool tryLockWrite() { return true; }

    /// Unlock the write lock.
    void unlockWrite() { }

    PRIME_UNCOPYABLE(NullReadWriteLock);
};
}

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsReadWriteLock.h"

namespace Prime {
typedef WindowsReadWriteLock ReadWriteLock;
typedef WindowsReadWriteLock TryReadWriteLock;
}

#elif 0

#include "Emulated/EmulatedReadWriteLock.h"

namespace Prime {
typedef EmulatedReadWriteLock ReadWriteLock;
// typedef EmulatedReadWriteLock TryReadWriteLock;
}

#elif defined(PRIME_OS_UNIX)

#include "Pthreads/PthreadsReadWriteLock.h"

namespace Prime {
typedef PthreadsReadWriteLock ReadWriteLock;
typedef PthreadsReadWriteLock TryReadWriteLock;
}

#else

namespace Prime {
typedef NullReadWriteLock ReadWriteLock;
typedef NullReadWriteLock TryReadWriteLock;
}

#endif

#endif
