// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_EMULATEDREADWRITELOCK_H
#define PRIME_EMULATEDREADWRITELOCK_H

#include "../Condition.h"
#include "../Log.h"
#include "../Mutex.h"
#include "../ScopedLock.h"

namespace Prime {

/// Emulate a read/write lock (read biased) using a recursive mutex and condition variables.
class PRIME_PUBLIC EmulatedReadWriteLock {
public:
    typedef Prime::ScopedReadLock<EmulatedReadWriteLock> ScopedReadLock;
    typedef Prime::ScopedWriteLock<EmulatedReadWriteLock> ScopedWriteLock;

    EmulatedReadWriteLock() { }

    explicit EmulatedReadWriteLock(Log* log, const char* debugName = NULL)
    {
        PRIME_EXPECT(init(log, debugName));
    }

    ~EmulatedReadWriteLock() { close(); }

    bool init(Log* log, const char* debugName = NULL);

    void close();

    bool isInitialised() const { return _mutex.isInitialised(); }

    /// Lock for reading.
    void lockRead();

    /// Try to lock for reading. Returns true if the lock was obtained.
    // bool tryLockRead();

    /// Unlock the read lock.
    void unlockRead();

    /// Lock for writing.
    void lockWrite();

    /// Try to lock for writing. Returns true if the lock was obtained.
    // bool tryLockWrite();

    /// Unlock the write lock.
    void unlockWrite();

private:
    RecursiveMutex _mutex;
    Condition _readerGate;
    Condition _writerGate;

    int _numReaders;
    int _numWriters;
    int _numWritersWaiting;

    PRIME_UNCOPYABLE(EmulatedReadWriteLock);
};
}

#endif
