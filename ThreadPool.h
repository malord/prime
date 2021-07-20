// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_THREADPOOL_H
#define PRIME_THREADPOOL_H

#include "Condition.h"
#include "Mutex.h"
#include "Semaphore.h"
#include "Thread.h"
#include "ThreadSpecificData.h"
#ifndef PRIME_CXX11_STL
#include "Callback.h"
#endif
#include <deque>
#include <functional>
#include <list>
#include <string>

namespace Prime {

/// A dynamically resizeable thread pool.
class PRIME_PUBLIC ThreadPool : public RefCounted {
public:
#ifdef PRIME_CXX11_STL
    typedef std::function<void()> Callback;
#else
    typedef Callback0<void> Callback;
#endif

    ThreadPool();

    ~ThreadPool();

    /// The Log is retained. If maxThreads or stackSize are zero, defaults are used. If maxConcurrent is zero,
    /// the number of CPUs is used. If maxConcurrent < 0, then -maxConcurrent * numberOfCPUs is used (e.g.,
    /// if maxConcurrent is -2 on a 4 core machine, there will be 8 threads).
    bool init(int maxConcurrent, int maxThreads, size_t stackSize, Log* log, const char* debugName = "ThreadPool");

    bool isInitialised() const { return _initialised; }

    void close();

    /// Run a task on the calling thread. This only needs to be done if the queue was created with zero threads.
    /// Returns true if there are more tasks queued.
    bool run(bool runAll = false, bool wait = false)
    {
        return runThread(runAll, wait, false);
    }

    /// Run a single task if one is waiting, otherwise return immediately. Returns true if any more tasks are
    /// now waiting.
    bool runOne()
    {
        return run(false, false);
    }

    void queue(const Callback& callback);

    bool isCallingThreadInPool() const
    {
        const ThreadData* data = getThreadData();
        return data && data->pool == this;
    }

    bool addThread();

    /// Blocks the calling thread until a thread has exited.
    void removeThread();

    /// Use RAII to ensure a thread that is added is removed.
    class ScopedAddThread {
    public:
        explicit ScopedAddThread(ThreadPool* pool)
        {
            _pool = (pool && pool->addThread()) ? pool : NULL;
        }

        ~ScopedAddThread()
        {
            if (_pool) {
                _pool->removeThread();
            }
        }

        void addThread(ThreadPool* pool)
        {
            removeThread();
            _pool = pool->addThread() ? pool : NULL;
        }

        void removeThread()
        {
            if (_pool) {
                _pool->removeThread();
                _pool = NULL;
            }
        }

    private:
        ThreadPool* _pool;

        PRIME_UNCOPYABLE(ScopedAddThread);
    };

    /// Returns the number of times enter() has been called on this thread, or -1 if this is not a ThreadPool
    /// thread. The very first call will return 1.
    int enter();

    void leave();

    class ScopedEnter {
    public:
        ScopedEnter(ThreadPool* pool)
        {
            _count = pool->enter();
            if (_count >= 0) {
                _pool = pool;
            } else {
                _pool = NULL;
            }
        }

        ~ScopedEnter()
        {
            if (_pool) {
                _pool->leave();
            }
        }

        int getCount() const { return _count; }

    private:
        int _count;
        ThreadPool* _pool;
    };

private:
    enum { defaultMaxThreads = 0 };
    enum { defaultStackSize = 128u * 1024u };

    static ThreadSpecificData* getThreadSpecificData();

    struct ThreadData {
        const ThreadPool* pool;
        int enterCount;
    };

    const ThreadData* getThreadData() const
    {
        return reinterpret_cast<const ThreadData*>(getThreadSpecificData()->get());
    }

    ThreadData* getThreadData()
    {
        return reinterpret_cast<ThreadData*>(getThreadSpecificData()->get());
    }

    bool createThread(RecursiveMutex::ScopedLock&);

    /// This is called by each of the threads we start.
    void thread();

    bool runThread(bool shouldRunAllTasks, bool shouldWaitForTasks, bool isThread);

    /// No longer wakes the threads, you must call _taskQueued.wakeOne() or _taskQueued.wakeAll()
    void internalQueue(const Callback& callback, RecursiveMutex::ScopedLock&);

    const char* getName() const;

    bool _initialised;

    RefPtr<Log> _log;

    RecursiveMutex _mutex;

    /// Incremented by createThread(), decremented by each thread when it exits.
    int _threadCount;

    Semaphore _concurrentSemaphore;
    Condition _taskQueued;
    Condition _threadExited;

    int _numberOfThreadsThatShouldExit;

    int _maxThreads;
    size_t _stackSize;

#if PRIME_MSC_AND_OLDER(1300)
    class Task {
    private:
        Callback _callback;

    public:
        Task(const Callback& initCallback)
            : _callback(initCallback)
        {
        }

        Task()
        {
        }

        bool isSet() const { return _callback.isSet(); }

        void invoke() const { return _callback(); }
    };
#else
    typedef Callback Task;
#endif

    std::deque<Task> _tasks;

    std::string _name;

    int _timeoutMilliseconds;
};
}

#endif
