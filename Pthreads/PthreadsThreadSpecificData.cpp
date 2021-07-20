// Copyright 2000-2021 Mark H. P. Lord

#include "PthreadsThreadSpecificData.h"
#include <string.h>

namespace Prime {

bool PthreadsThreadSpecificData::init(Log* log, ThreadDestroyedCallback threadDestroyedCallback, const char* debugName)
{
    PRIME_ASSERT(!_initialised);

    _callback = threadDestroyedCallback;

    memset(&_key, 0, sizeof(_key));

    int result = pthread_key_create(&_key, threadDestroyedCallback);
    _initialised = result == 0;
    if (!_initialised) {
        log->logErrno(result, debugName);
        return false;
    }

    return true;
}

PthreadsThreadSpecificData::~PthreadsThreadSpecificData()
{
    if (_initialised) {
        clear();
        PRIME_EXPECT(pthread_key_delete(_key) == 0);
    }
}

void PthreadsThreadSpecificData::set(void* data)
{
    PRIME_ASSERT(_initialised);
    PRIME_EXPECT(pthread_setspecific(_key, data) == 0);
}

void* PthreadsThreadSpecificData::get()
{
    PRIME_ASSERT(_initialised);
    return pthread_getspecific(_key);
}

void PthreadsThreadSpecificData::clear()
{
    PRIME_ASSERT(_initialised);
    void* data = get();
    if (data && _callback) {
        (*_callback)(data);
    }

    set(0);
}

}
