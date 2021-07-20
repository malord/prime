// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSCONDITION_H
#define PRIME_WINDOWS_WINDOWSCONDITION_H

#include "../Log.h"
#include "../Timeout.h"
#include "WindowsConfig.h"

namespace Prime {

/// Windows emulation of condition variables (condition variables weren't added to Windows until Vista).
class PRIME_PUBLIC WindowsCondition {
public:
    WindowsCondition() { construct(); }

    ~WindowsCondition();

    template <typename Mutex>
    WindowsCondition(Mutex* mutex, Log* log, const char* debugName = NULL)
    {
        construct();
        PRIME_EXPECT(init(mutex, log, debugName));
    }

    explicit WindowsCondition(Log* log, const char* debugName = NULL)
    {
        construct();
        PRIME_EXPECT(init(log, debugName));
    }

    /// On some platforms, the Mutex must be specified at the time the condition variable is created. On
    /// Windows, ignore the mutex.
    template <typename Mutex>
    bool init(Mutex*, Log* log, const char* debugName = NULL)
    {
        return init(log, debugName);
    }

    bool init(Log* log, const char* debugName = NULL);

    void close();

    bool isInitialised() const { return _events.event.all != NULL; }

    /// Wake a single waiting thread.
    void wakeOne() { notify(0); }

    /// Wake all waiting threads.
    void wakeAll() { notify(1); }

    /// Wait for the condition, up to the specified number of milliseconds. Returns true if the condition was met
    /// and the lock obtained.
    template <typename MutexScopedLock>
    bool timedWait(MutexScopedLock& scopedLock, int milliseconds)
    {
        PRIME_ASSERT(isInitialised());

        Timeout timeout(milliseconds);

        EnterCriticalSection(&_lock);
        ++_waitersCount;
        LeaveCriticalSection(&_lock);

        scopedLock.getLockable()->unlock();

        DWORD result = WaitForMultipleObjects(2, _events.array, FALSE, timeout.getWindowsMillisecondsRemaining());
        if (result == WAIT_TIMEOUT) {
            return false;
        }

        if (result == WAIT_FAILED) {
            return false;
        }

        EnterCriticalSection(&_lock);
        --_waitersCount;
        bool wasLastWaiter = result == (WAIT_OBJECT_0 + 1) && _waitersCount == 0;
        LeaveCriticalSection(&_lock);

        if (wasLastWaiter) {
            ResetEvent(_events.event.all);
        }

        scopedLock.getLockable()->lock();
        return true;
    }

    /// Wait for the condition. Once the condition is met, lock the specified lock.
    template <typename MutexScopedLock>
    void wait(MutexScopedLock& scopedLock)
    {
        PRIME_ASSERT(isInitialised());

        timedWait(scopedLock, -1);
    }

private:
    void construct();

    void notify(int which);

    union {
        struct {
            HANDLE one;
            HANDLE all;
        } event;
        HANDLE array[2];
    } _events;

    int _waitersCount;
    CRITICAL_SECTION _lock;
};
}

#endif
