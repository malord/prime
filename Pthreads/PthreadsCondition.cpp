// Copyright 2000-2021 Mark H. P. Lord

#include "PthreadsCondition.h"
#include "PthreadsTime.h"
#include <string.h>

namespace Prime {

bool PthreadsCondition::init(Log* log, const char* debugName)
{
    PRIME_ASSERT(!_initialised); // call close() first.

    memset(&_condition, 0, sizeof(_condition));
    int result = pthread_cond_init(&_condition, 0);
    _initialised = result == 0;
    if (!_initialised) {
        log->logErrno(result, debugName);
    }

    return _initialised;
}

void PthreadsCondition::close()
{
    if (_initialised) {
        PRIME_EXPECT(pthread_cond_destroy(&_condition) == 0);

        _initialised = false;
    }
}

bool PthreadsCondition::pthreadsTimedWait(pthread_mutex_t* mutex, int milliseconds)
{
    PRIME_ASSERT(_initialised);

    timespec ts;
    if (!ComputeTimespecAfterMilliseconds(milliseconds, ts)) {
        return false;
    }

    return pthread_cond_timedwait(&_condition, mutex, &ts) == 0;
}

}
