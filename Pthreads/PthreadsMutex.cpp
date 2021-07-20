// Copyright 2000-2021 Mark H. P. Lord

#include "PthreadsMutex.h"
#include <string.h>

namespace Prime {

bool PthreadsMutex::init(Log* log, const char* debugName, const Attributes& attributes)
{
    PRIME_ASSERT(!_initialised); // call close() first.

    pthread_mutexattr_t attr;
    memset(&attr, 0, sizeof(attr));
    memset(&_mutex, 0, sizeof(_mutex));

    int result;

#if !defined(PRIME_FINAL) && defined(PTHREAD_MUTEX_ERRORCHECK)
#define PRIME_PTHREAD_MUTEX_NORMAL PTHREAD_MUTEX_ERRORCHECK
#else
#define PRIME_PTHREAD_MUTEX_NORMAL PTHREAD_MUTEX_NORMAL
#endif

    if ((result = pthread_mutexattr_init(&attr)) == 0) {
        if ((result = pthread_mutexattr_settype(&attr, attributes.getRecursive() ? PTHREAD_MUTEX_RECURSIVE : PRIME_PTHREAD_MUTEX_NORMAL)) == 0) {
            if ((result = pthread_mutex_init(&_mutex, &attr)) == 0) {
                _initialised = true;
                pthread_mutexattr_destroy(&attr);
                return true;
            }
        }

        pthread_mutexattr_destroy(&attr);
    }

    log->logErrno(result, debugName);
    return false;
}

void PthreadsMutex::close()
{
    if (_initialised) {
        PRIME_EXPECT(pthread_mutex_destroy(&_mutex) == 0);

        _initialised = false;
    }
}

}
