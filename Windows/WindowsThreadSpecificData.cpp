// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsThreadSpecificData.h"

namespace Prime {

bool WindowsThreadSpecificData::init(Log* log, ThreadDestroyedCallback threadDestroyedCallback, const char* debugName)
{
    PRIME_ASSERT(!isInitialised());

    _tls = TlsAlloc();
    if (_tls == TLS_OUT_OF_INDEXES) {
        log->logWindowsError(GetLastError(), debugName);
        return false;
    }

    _callback = threadDestroyedCallback;
    return true;
}

WindowsThreadSpecificData::~WindowsThreadSpecificData()
{
    if (isInitialised()) {
        clear();
        TlsFree(_tls);
    }
}

void WindowsThreadSpecificData::set(void* data)
{
    PRIME_ASSERT(isInitialised());
    TlsSetValue(_tls, data);
}

void* WindowsThreadSpecificData::get()
{
    PRIME_ASSERT(isInitialised());
    return TlsGetValue(_tls);
}

void WindowsThreadSpecificData::clear()
{
    PRIME_ASSERT(isInitialised());

    void* data = get();
    if (data && _callback) {
        (*_callback)(data);
    }

    set(0);
}

}
