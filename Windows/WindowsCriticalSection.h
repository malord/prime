// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSCRITICALSECTION_H
#define PRIME_WINDOWS_WINDOWSCRITICALSECTION_H

#include "../Log.h"
#include "../ScopedLock.h"
#include "WindowsConfig.h"

namespace Prime {

/// A wrapper around a Windows CRITICAL_SECTION (a relatively lightweight recursive mutex).
class WindowsCriticalSection {
public:
    typedef Prime::ScopedLock<WindowsCriticalSection> ScopedLock;

    WindowsCriticalSection() { InitializeCriticalSection(&_cs); }

    explicit WindowsCriticalSection(Log*, const char* = NULL) { InitializeCriticalSection(&_cs); }

    ~WindowsCriticalSection() { DeleteCriticalSection(&_cs); }

    bool init(Log* log, const char* debugName = NULL)
    {
        (void)log;
        (void)debugName;
        return true;
    }

    // No-op - for compatibility with Pthreads.
    bool isInitialised() const { return true; }

    // No-op - for compatibility with Pthreads.
    void close() { }

    void lock() { EnterCriticalSection(&_cs); }

    void unlock() { LeaveCriticalSection(&_cs); }

private:
    CRITICAL_SECTION _cs;

    PRIME_UNCOPYABLE(WindowsCriticalSection);
};

}

#endif
