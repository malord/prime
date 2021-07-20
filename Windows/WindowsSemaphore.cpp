// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsSemaphore.h"

namespace Prime {

bool WindowsSemaphore::init(LONG initialCount, Log* log, const char* debugName, LONG maximumCount)
{
    PRIME_ASSERT(!isInitialised()); // call close() first.

    _semaphore = CreateSemaphore(0, initialCount, maximumCount, 0);
    if (!_semaphore) {
        log->logWindowsError(GetLastError(), debugName);
        return false;
    }

    return true;
}

void WindowsSemaphore::close()
{
    if (_semaphore) {
        CloseHandle(_semaphore);

        _semaphore = 0;
    }
}

void WindowsSemaphore::lock(LONG n)
{
    while (n-- > 0) {
        lock();
    }
}
}
