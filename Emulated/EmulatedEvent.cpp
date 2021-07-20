// Copyright 2000-2021 Mark H. P. Lord

#include "EmulatedEvent.h"
#include "../Timeout.h"

namespace Prime {

bool EmulatedEvent::init(bool initiallySet, bool manualReset, Log* log, const char* debugName)
{
    if (!_mutex.init(log, debugName) || !_condition.init(&_mutex, log, debugName)) {
        return false;
    }

    _set = initiallySet;
    _manualReset = manualReset;
    return true;
}

void EmulatedEvent::close()
{
    _mutex.close();
    _condition.close();
}

void EmulatedEvent::set()
{
    PRIME_ASSERT(isInitialised());
    RecursiveMutex::ScopedLock lock(&_mutex);

    if (!_set) {
        _set = true;

        if (_manualReset) {
            _condition.wakeAll();
        } else {
            _condition.wakeOne();
        }
    }
}

void EmulatedEvent::wait()
{
    PRIME_ASSERT(isInitialised());
    RecursiveMutex::ScopedLock lock(&_mutex);

    while (!_set) {
        _condition.wait(lock);
    }

    if (!_manualReset) {
        _set = false;
    }
}

bool EmulatedEvent::tryWait(int milliseconds)
{
    PRIME_ASSERT(isInitialised());
    RecursiveMutex::ScopedLock lock(&_mutex);

    Timeout timeout(milliseconds);

    while (!_set) {
        if (!_condition.timedWait(lock, timeout.getMillisecondsRemaining())) {
            if (!_set) {
                return false;
            }
        }
    }

    if (!_manualReset) {
        _set = false;
    }

    return true;
}

void EmulatedEvent::reset()
{
    PRIME_ASSERT(isInitialised());
    RecursiveMutex::ScopedLock lock(&_mutex);

    _set = false;
}
}
