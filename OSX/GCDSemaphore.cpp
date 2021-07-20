// Copyright 2000-2021 Mark H. P. Lord

#include "GCDSemaphore.h"

namespace Prime {

bool GCDSemaphore::init(long initialCount, Log* log, const char* debugName, long /*maximumCount*/)
{
    if (!PRIME_GUARD(!_semaphore)) {
        return true;
    }

    (void)log;
    (void)debugName;

    _semaphore = dispatch_semaphore_create(initialCount);

    return true;
}

void GCDSemaphore::close()
{
    if (_semaphore) {
        dispatch_release(_semaphore);
        _semaphore = NULL;
    }
}

void GCDSemaphore::lock(long n)
{
    while (n-- > 0) {
        lock();
    }
}

void GCDSemaphore::post(long increment)
{
    while (increment-- > 0) {
        unlock();
    }
}

bool GCDSemaphore::tryLock(int milliseconds)
{
    PRIME_ASSERT(isInitialised());
    dispatch_time_t timeout = milliseconds < 0 ? DISPATCH_TIME_FOREVER : dispatch_time(0, static_cast<int64_t>(milliseconds)) * 1000000;
    return dispatch_semaphore_wait(_semaphore, timeout) == 0;
}
}
