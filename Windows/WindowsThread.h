// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSTHREAD_H
#define PRIME_WINDOWS_WINDOWSTHREAD_H

#include "../Log.h"
#include "WindowsConfig.h"
#ifndef PRIME_CXX11_STL
#include "../Callback.h"
#endif
#include <functional>

namespace Prime {

class PRIME_PUBLIC WindowsThread : public RefCounted {
public:
#ifdef PRIME_CXX11_STL
    typedef std::function<void()> Callback;
#else
    typedef Callback0<void> Callback;
#endif

    typedef DWORD ThreadID;

    static DWORD getCallingThreadID() { return GetCurrentThreadId(); }

    static int getCPUCount(Log* log);

    WindowsThread();

    ~WindowsThread();

    /// Launch a thread. Specify zero to use the default stackSize.
    bool create(void (*entryPoint)(void*), void* context, size_t stackSize, Log* log, const char* debugName = NULL);

    /// Launch a thread. Specify zero to use the default stackSize.
    bool create(const Callback& callback, size_t stackSize, Log* log, const char* debugName = NULL);

    /// Return true if a thread has been started and is still running.
    bool isRunning();

    /// Wait for the thread to exit, return false on error. It's safe to call this when the thread has already
    /// finished.
    bool join();

    /// Attempt to forcibly abort the thread. Not available on all platforms.
    bool cancel();

    ThreadID getThreadID() const { return _threadID; }

private:
    HANDLE _handle;
    unsigned int _threadID;

    PRIME_UNCOPYABLE(WindowsThread);
};

}

#endif
