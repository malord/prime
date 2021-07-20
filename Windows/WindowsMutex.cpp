// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsMutex.h"

namespace Prime {

bool WindowsMutex::init(Log* log, const char* debugName)
{
    PRIME_ASSERT(!isInitialised()); // call close() first.

    _mutex = CreateMutex(0, FALSE, 0);
    if (!_mutex) {
        log->logWindowsError(GetLastError(), debugName);
        return false;
    }

    return true;
}

}
