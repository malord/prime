// Copyright 2000-2021 Mark H. P. Lord

#include "PthreadsSemaphore.h"

#ifdef PRIME_OS_HAS_PTHREADSSEMAPHORE

#include "PthreadsTime.h"
#include <errno.h>
#include <string.h>

namespace Prime {

bool PthreadsSemaphore::init(int initialCount, Log* log, const char* debugName, int maximumCount)
{
    PRIME_ASSERT(!_initialised); // call close() first.

    (void)maximumCount;

    memset(&_sem, 0, sizeof(_sem));
    if (sem_init(&_sem, 0, initialCount) == -1) {
        log->logErrno(errno, debugName);
        return false;
    }

    _initialised = true;
    return true;
}

void PthreadsSemaphore::close()
{
    if (_initialised) {
        sem_destroy(&_sem);

        _initialised = false;
    }
}

void PthreadsSemaphore::lock()
{
    PRIME_ASSERT(_initialised);

    for (;;) {
        if (sem_wait(&_sem) == 0) {
            break;
        }

        if (errno != EINTR) {
            RuntimeError("sem_wait errno %d", (int)errno);
            break;
        }
    }
}

void PthreadsSemaphore::lock(int n)
{
    while (n--) {
        lock();
    }
}

void PthreadsSemaphore::post(int increment)
{
    PRIME_ASSERT(_initialised);

    while (increment--) {
        sem_post(&_sem);
    }
}

bool PthreadsSemaphore::tryLock()
{
    PRIME_ASSERT(_initialised);

    return sem_trywait(&_sem) == 0;
}

bool PthreadsSemaphore::tryLock(int milliseconds)
{
    PRIME_ASSERT(isInitialised());

    timespec ts;
    if (!ComputeTimespecAfterMilliseconds(milliseconds, ts)) {
        return false;
    }

    if (sem_timedwait(&_sem, &ts) == 0) {
        return true;
    }

    return false;
}
}

#endif
