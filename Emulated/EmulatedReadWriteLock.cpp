// Copyright 2000-2021 Mark H. P. Lord

// tryLockWrite and tryLockRead are not hard to implement, but I've not needed them.

#include "EmulatedReadWriteLock.h"

namespace Prime {

bool EmulatedReadWriteLock::init(Log* log, const char* debugName)
{
    _numReaders = 0;
    _numWriters = 0;
    _numWritersWaiting = 0;

    if (!_mutex.init(log, debugName) || !_readerGate.init(&_mutex, log, debugName) || !_writerGate.init(&_mutex, log, debugName)) {

        return false;
    }

    return true;
}

void EmulatedReadWriteLock::close()
{
    _mutex.close();
    _readerGate.close();
    _writerGate.close();
}

void EmulatedReadWriteLock::lockRead()
{
    PRIME_ASSERT(isInitialised());
    RecursiveMutex::ScopedLock lock(&_mutex);
    while (_numWriters > 0) {
        _readerGate.wait(lock);
    }
    ++_numReaders;
}

// bool EmulatedReadWriteLock::tryLockRead()
// {
// }

void EmulatedReadWriteLock::unlockRead()
{
    PRIME_ASSERT(isInitialised());
    RecursiveMutex::ScopedLock lock(&_mutex);
    --_numReaders;
    if (_numReaders == 0 && _numWritersWaiting > 0) {
        _writerGate.wakeOne();
    }
}

void EmulatedReadWriteLock::lockWrite()
{
    PRIME_ASSERT(isInitialised());
    RecursiveMutex::ScopedLock lock(&_mutex);
    ++_numWritersWaiting;
    while (_numReaders > 0 || _numWriters > 0) {
        _writerGate.wait(lock);
    }
    --_numWritersWaiting;
    ++_numWriters;
}

// bool EmulatedReadWriteLock::tryLockWrite()
// {
// }

void EmulatedReadWriteLock::unlockWrite()
{
    PRIME_ASSERT(isInitialised());
    RecursiveMutex::ScopedLock lock(&_mutex);
    --_numWriters;
    if (_numWritersWaiting > 0) {
        _writerGate.wakeOne();
    }
    _readerGate.wakeAll();
}
}
