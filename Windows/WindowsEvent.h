// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSEVENT_H
#define PRIME_WINDOWS_WINDOWSEVENT_H

#include "../Log.h"
#include "WindowsConfig.h"

namespace Prime {

/// Emulate a Windows Event threading primitive on other platforms.
class PRIME_PUBLIC WindowsEvent {
public:
    WindowsEvent()
        : _event(NULL)
    {
    }

    WindowsEvent(bool initiallySet, bool manualReset, Log* log, const char* debugName = NULL)
        : _event(NULL)
    {
        PRIME_EXPECT(init(initiallySet, manualReset, log, debugName));
    }

    ~WindowsEvent() { close(); }

    bool init(bool initiallySet, bool manualReset, Log* log, const char* debugName = NULL);

    void close();

    bool isInitialised() { return _event != NULL; }

    void set()
    {
        PRIME_DEBUG_ASSERT(isInitialised());
        SetEvent(_event);
    }

    /// Wait for the event to be set.
    void wait()
    {
        PRIME_DEBUG_ASSERT(isInitialised());
        WaitForSingleObject(_event, INFINITE);
    }

    void reset()
    {
        PRIME_DEBUG_ASSERT(isInitialised());
        ResetEvent(_event);
    }

    bool tryWait(int milliseconds = 0)
    {
        PRIME_DEBUG_ASSERT(isInitialised());
        return WaitForSingleObject(_event, milliseconds < 0 ? INFINITE : (DWORD)milliseconds) == WAIT_OBJECT_0;
    }

    void unlock() { set(); }

    void lock() { wait(); }

    bool tryLock(int milliseconds = 0) { return tryWait(milliseconds); }

    // bool isSet() const { return _set; }

    // No pulseEvent() - use a condition variable

private:
    HANDLE _event;
};
}

#endif
