// Copyright 2000-2021 Mark H. P. Lord

#include "PthreadsRecursiveTimedMutex.h"
#include "PthreadsTime.h"
#include <errno.h>
#include <string.h>

namespace Prime {

bool PthreadsRecursiveTimedMutex::init(Log* log, const char* debugName)
{
    PRIME_ASSERT(!_initialised); // call close() first.

    memset(&_unlocked, 0, sizeof(_unlocked));
    memset(&_mutex, 0, sizeof(_mutex));

    int result;
    if ((result = pthread_mutex_init(&_mutex, 0)) == 0) {
        if ((result = pthread_cond_init(&_unlocked, 0)) == 0) {
            _initialised = true;
        } else {
            pthread_mutex_destroy(&_mutex);
            log->logErrno(result, debugName);
        }
    } else {
        log->logErrno(result, debugName);
    }

    _locked = false;
    _reentered = 0;

    return _initialised;
}

void PthreadsRecursiveTimedMutex::close()
{
    if (_initialised) {
        PRIME_EXPECT(pthread_mutex_destroy(&_mutex) == 0);
        PRIME_EXPECT(pthread_cond_destroy(&_unlocked) == 0);

        _initialised = false;
    }
}

void PthreadsRecursiveTimedMutex::lock()
{
    PRIME_ASSERT(_initialised);

    PRIME_EXPECT(pthread_mutex_lock(&_mutex) == 0);

    pthread_t thisThread = pthread_self();
    if (_locked && pthread_equal(_lockedByThread, thisThread)) {
        ++_reentered;
    } else {
        while (_locked) {
            PRIME_EXPECT(pthread_cond_wait(&_unlocked, &_mutex) == 0);
        }

        _lockedByThread = thisThread;
        _locked = true;
        _reentered = 1;
    }

    PRIME_EXPECT(pthread_mutex_unlock(&_mutex) == 0);
}

void PthreadsRecursiveTimedMutex::unlock()
{
    PRIME_ASSERT(_initialised);

    PRIME_EXPECT(pthread_mutex_lock(&_mutex) == 0);

    pthread_t thisThread = pthread_self();
    if (_locked && !pthread_equal(_lockedByThread, thisThread)) {
        // Wrong thread unlocked us.
        PRIME_ASSERT(pthread_equal(_lockedByThread, thisThread));
        PRIME_EXPECT(pthread_mutex_unlock(&_mutex) == 0);
        return;
    }

    if (!--_reentered) {
        _locked = false;
        PRIME_EXPECT(pthread_cond_signal(&_unlocked) == 0);
    }

    PRIME_EXPECT(pthread_mutex_unlock(&_mutex) == 0);
}

bool PthreadsRecursiveTimedMutex::tryLock()
{
    PRIME_ASSERT(_initialised);

    PRIME_EXPECT(pthread_mutex_lock(&_mutex) == 0);

    pthread_t thisThread = pthread_self();
    bool didLock;

    if (_locked && pthread_equal(_lockedByThread, thisThread)) {
        ++_reentered;
        didLock = true;
    } else if (!_locked) {
        _lockedByThread = thisThread;
        _locked = true;
        _reentered = 1;
        didLock = true;
    } else {
        didLock = false;
    }

    PRIME_EXPECT(pthread_mutex_unlock(&_mutex) == 0);
    return didLock;
}

bool PthreadsRecursiveTimedMutex::tryLock(int milliseconds)
{
    PRIME_ASSERT(_initialised);

    timespec ts;
    if (!ComputeTimespecAfterMilliseconds(milliseconds, ts)) {
        return false;
    }

    PRIME_EXPECT(pthread_mutex_lock(&_mutex) == 0);

    pthread_t thisThread = pthread_self();
    bool didLock;

    if (_locked && pthread_equal(_lockedByThread, thisThread)) {
        ++_reentered;
        didLock = true;
    } else {
        while (_locked) {
            int result = pthread_cond_timedwait(&_unlocked, &_mutex, &ts);
            if (result == ETIMEDOUT) {
                break;
            }
            PRIME_EXPECT(result == 0);
        }

        if (!_locked) {
            _lockedByThread = thisThread;
            _locked = true;
            _reentered = 1;
            didLock = true;
        } else {
            didLock = false;
        }
    }

    PRIME_EXPECT(pthread_mutex_unlock(&_mutex) == 0);
    return didLock;
}

}
