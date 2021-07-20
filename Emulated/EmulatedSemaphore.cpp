// Copyright 2000-2021 Mark H. P. Lord

#include "EmulatedSemaphore.h"
#include "../Timeout.h"
#include <string.h>

namespace Prime {

bool EmulatedSemaphore::init(int initialCount, Log* log, const char* debugName, int)
{
    PRIME_ASSERT(!isInitialised()); // call close() first.

    if (!_mutex.init(log, debugName) || !_nonzero.init(&_mutex, log, debugName)) {

        close();
        return false;
    }

    _count = initialCount;
    _numberWaiting = 0;

    return true;
}

void EmulatedSemaphore::close()
{
    _nonzero.close();
    _mutex.close();
}

void EmulatedSemaphore::lock(int n)
{
    PRIME_ASSERT(isInitialised());

    NonRecursiveMutex::ScopedLock lockMutex(&_mutex);

    _numberWaiting++;

    while (n--) {
        while (!_count) {
            _nonzero.wait(lockMutex);
        }

        _count--;
    }

    _numberWaiting--;
}

bool EmulatedSemaphore::post(int increment)
{
    PRIME_ASSERT(isInitialised());
    PRIME_ASSERT(increment >= 0);

    if (!increment) {
        return false;
    }

    NonRecursiveMutex::ScopedLock lockMutex(&_mutex);

    bool anyWaiters = false;

    if (_numberWaiting) {
        _nonzero.wakeAll();
        anyWaiters = true;
    }

    _count += increment;
    return anyWaiters;
}

bool EmulatedSemaphore::tryLock()
{
    PRIME_ASSERT(isInitialised());

    NonRecursiveMutex::ScopedLock lockMutex(&_mutex);

    bool locked;

    if (_count) {
        _count--;
        locked = true;
    } else {
        locked = false;
    }

    return locked;
}

bool EmulatedSemaphore::tryLock(int milliseconds)
{
    PRIME_ASSERT(isInitialised());

    Timeout timeout(milliseconds);

    NonRecursiveMutex::ScopedLock lockMutex(&_mutex);

    _numberWaiting++;

    bool obtained = true;

    while (!_count) {
        if (!_nonzero.timedWait(lockMutex, timeout.getMillisecondsRemaining())) {
            obtained = false;
            break;
        }
    }

    _numberWaiting--;

    if (obtained) {
        _count--;
    }

    return obtained;
}

}
