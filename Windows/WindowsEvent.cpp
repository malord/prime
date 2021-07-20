// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsEvent.h"
#include "../Timeout.h"

namespace Prime {

bool WindowsEvent::init(bool initiallySet, bool manualReset, Log* log, const char* debugName)
{
    PRIME_ASSERT(!isInitialised());

    (void)debugName;

    _event = CreateEvent(NULL, manualReset ? TRUE : FALSE, initiallySet ? TRUE : FALSE, NULL);
    if (!_event) {
        log->logWindowsError(GetLastError(), "CreateEvent");
        return false;
    }

    return true;
}

void WindowsEvent::close()
{
    if (_event) {
        CloseHandle(_event);
        _event = NULL;
    }
}
}
