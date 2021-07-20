// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsCondition.h"

namespace Prime {

void WindowsCondition::construct()
{
    _events.event.one = NULL;
    _events.event.all = NULL;
    InitializeCriticalSection(&_lock);
}

WindowsCondition::~WindowsCondition()
{
    close();
    DeleteCriticalSection(&_lock);
}

bool WindowsCondition::init(Log* log, const char* debugName)
{
    (void)debugName;
    PRIME_ASSERT(!isInitialised());

    _waitersCount = 0;

    _events.event.one = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!_events.event.one) {
        log->logWindowsError(GetLastError(), "CreateEvent (1)");
        return false;
    }

    _events.event.all = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!_events.event.all) {
        log->logWindowsError(GetLastError(), "CreateEvent (2)");
        CloseHandle(_events.event.one);
        _events.event.one = NULL;

        return false;
    }

    return true;
}

void WindowsCondition::close()
{
    for (int i = 0; i != 2; ++i) {
        if (_events.array[i]) {
            CloseHandle(_events.array[i]);
            _events.array[i] = NULL;
        }
    }
}

void WindowsCondition::notify(int which)
{
    EnterCriticalSection(&_lock);
    bool anyoneWaiting = _waitersCount > 0;
    LeaveCriticalSection(&_lock);

    if (anyoneWaiting) {
        SetEvent(_events.array[which]);
    }
}

}
