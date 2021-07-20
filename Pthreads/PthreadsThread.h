// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PTHREADS_PTHREADSTHREAD_H
#define PRIME_PTHREADS_PTHREADSTHREAD_H

#include "../Log.h"
#ifndef PRIME_CXX11_STL
#include "../Callback.h"
#endif
#include <functional>
#include <pthread.h>

namespace Prime {

/// Wrap a pthreads thread handle.
class PRIME_PUBLIC PthreadsThread : public RefCounted {
public:
#ifdef PRIME_CXX11_STL
    typedef std::function<void()> Callback;
#else
    typedef Callback0<void> Callback;
#endif

    class ThreadID {
    public:
        ThreadID(const ThreadID& copy) { operator=(copy); }

        bool operator==(const ThreadID& other) const
        {
            return pthread_equal(_threadID, other._threadID);
        }

        bool operator!=(const ThreadID& other) const
        {
            return !operator==(other);
        }

        pthread_t getPthreadID() const { return _threadID; }

    private:
        friend class PthreadsThread;

        explicit ThreadID(pthread_t threadID)
            : _threadID(threadID)
        {
        }

        pthread_t _threadID;
    };

    static ThreadID getCallingThreadID() { return ThreadID(pthread_self()); }

    static int getCPUCount(Log* log);

    PthreadsThread();

    ~PthreadsThread();

    /// Launch a thread (non-Callback version for backwards compatibility). Specify zero to use the default
    /// stackSize.
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

    const ThreadID getThreadID() { return ThreadID(_thread); }

private:
    /// Passes control from the thread to the callback.
    static void* thunk(void* data);

    pthread_t _thread;
    bool _attached;

    PRIME_UNCOPYABLE(PthreadsThread);
};

}

#endif
