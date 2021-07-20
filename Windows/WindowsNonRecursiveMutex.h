// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSNONRECURSIVEMUTEX_H
#define PRIME_WINDOWS_WINDOWSNONRECURSIVEMUTEX_H

#include "../ScopedLock.h"
#include "WindowsSemaphore.h"

namespace Prime {

/// A non recursive mutex. On Windows, this is implemented as a semaphore with a count of one.
class WindowsNonRecursiveMutex : public WindowsSemaphore {
public:
    typedef Prime::ScopedLock<WindowsNonRecursiveMutex> ScopedLock;

    WindowsNonRecursiveMutex() { }

    explicit WindowsNonRecursiveMutex(Log* log, const char* debugName = NULL)
        : WindowsSemaphore(1, log, debugName)
    {
    }

    bool init(Log* log, const char* debugName = NULL)
    {
        return WindowsSemaphore::init(1, log, debugName, 1);
    }

    PRIME_UNCOPYABLE(WindowsNonRecursiveMutex);
};

}

#endif
