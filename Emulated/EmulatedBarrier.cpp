// Copyright 2000-2021 Mark H. P. Lord

#include "EmulatedBarrier.h"

namespace Prime {

bool EmulatedBarrier::init(int count, Log* log, const char* debugName)
{
    PRIME_ASSERT(count > 0);

    _count = count;
    _waiting.set(0);

    if (!_semaphore.init(0, log, debugName)) {
        return false;
    }

    return true;
}

void EmulatedBarrier::close()
{
    _semaphore.close();
}

void EmulatedBarrier::wait()
{
    PRIME_ASSERT(isInitialised());

    if (_waiting.increment() == _count) {
        if (_count > 1) {
            _semaphore.post(_count - 1);
        }
    } else {
        _semaphore.lock();
    }
}
}
