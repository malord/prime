// Copyright 2000-2021 Mark H. P. Lord

#include "PthreadsReadWriteLock.h"
#include <string.h>

namespace Prime {

bool PthreadsReadWriteLock::init(Log* log, const char* debugName)
{
    PRIME_ASSERT(!_initialised); // call close() first.

    memset(&_rwlock, 0, sizeof(_rwlock));

    int result = pthread_rwlock_init(&_rwlock, 0);
    _initialised = result == 0;
    if (!_initialised) {
        log->logErrno(result, debugName);
    }

    return _initialised;
}

void PthreadsReadWriteLock::close()
{
    if (_initialised) {
        PRIME_EXPECT(pthread_rwlock_destroy(&_rwlock) == 0);

        _initialised = false;
    }
}

}
