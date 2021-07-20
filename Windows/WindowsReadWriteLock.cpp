// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsReadWriteLock.h"
#include "../Timeout.h"

namespace Prime {

bool WindowsReadWriteLock::init(Log* log, const char* debugName)
{
    PRIME_ASSERT(!isInitialised()); // call close() first.

    _readEvent = CreateEvent(0, TRUE, FALSE, 0);
    if (!_readEvent) {
        log->logWindowsError(GetLastError(), debugName);
        return false;
    }

    _writeMutex = CreateMutex(0, FALSE, 0);
    if (!_writeMutex) {
        CloseHandle(_readEvent);
        _readEvent = 0;

        log->logWindowsError(GetLastError(), debugName);
        return false;
    }

    _readerCount = 0;
    return true;
}

void WindowsReadWriteLock::close()
{
    if (_readEvent) {
        CloseHandle(_readEvent);
        _readEvent = 0;
    }

    if (_writeMutex) {
        CloseHandle(_writeMutex);
        _writeMutex = 0;
    }
}

bool WindowsReadWriteLock::tryLockRead(int milliseconds)
{
    PRIME_ASSERT(isInitialised());

    if (WaitForSingleObject(_writeMutex, milliseconds < 0 ? INFINITE : (DWORD)milliseconds) != WAIT_OBJECT_0) {
        return false;
    }

    InterlockedIncrement(&_readerCount);

    ResetEvent(_readEvent);
    ReleaseMutex(_writeMutex);

    return true;
}

void WindowsReadWriteLock::unlockRead()
{
    PRIME_ASSERT(isInitialised());

    // Make sure we're locked for reading
    PRIME_ASSERT(_readerCount);

    if (_readerCount) {
        InterlockedDecrement(&_readerCount);
        SetEvent(_readEvent);
    }
}

bool WindowsReadWriteLock::tryLockWrite(int milliseconds)
{
    PRIME_ASSERT(isInitialised());

    Timeout timeout(milliseconds);

    if (WaitForSingleObject(_writeMutex, timeout.getWindowsMillisecondsRemaining()) != WAIT_OBJECT_0) {
        return false;
    }

    if (_readerCount) {
        if (WaitForSingleObject(_readEvent, timeout.getWindowsMillisecondsRemaining()) != WAIT_OBJECT_0) {
            ReleaseMutex(_writeMutex);
            return false;
        }
    }

    return true;
}

void WindowsReadWriteLock::unlockWrite()
{
    PRIME_ASSERT(isInitialised());

    BOOL ok = ReleaseMutex(_writeMutex);
    ok = ok;

    // If the ReleaseMutex failed, we probably weren't locked for writing.
    PRIME_ASSERT(ok);
}

}
