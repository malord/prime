// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_THREADPOOLTASKSYSTEM_H
#define PRIME_THREADPOOLTASKSYSTEM_H

#include "TaskSystem.h"
#include "ThreadPool.h"

namespace Prime {

/// A TaskSystem implementation that uses a ThreadPool.
class PRIME_PUBLIC ThreadPoolTaskSystem : public TaskSystem {
public:
    ThreadPoolTaskSystem();

    ~ThreadPoolTaskSystem();

    /// The Log and ThreadPool are retained.
    bool init(ThreadPool* threadPool, Log* log);

    /// Construct the ThreadPool for you. If you're using ThreadPoolTaskSystem via DefaultTaskSystem, use this
    /// init() as it will exist in all TaskSystems that might be typedef'd to DefaultTaskSystem.
    bool init(int concurrentThreadCount, int maxThreadCount, size_t stackSize, Log* log);

    /// Must be called after init().
    void setMainQueueTaskQueuedCallback(const TaskQueue::Callback& value);

    void close();

    // TaskSystem implementation.
    virtual TaskQueue* getConcurrentQueue() PRIME_OVERRIDE { return _concurrentQueue; }
    virtual TaskQueue* getMainThreadQueue() PRIME_OVERRIDE { return _mainQueue; }
    virtual void runMainThreadQueue() PRIME_OVERRIDE;
    virtual bool isMainThread() PRIME_OVERRIDE;
    virtual RefPtr<TaskQueue> createSerialQueue() PRIME_OVERRIDE;
    virtual bool yieldDoNotCallDirectly() PRIME_OVERRIDE;
    virtual void resumeDoNotCallDirectly() PRIME_OVERRIDE;

    ThreadPool* getThreadPool() const { return _threadPool; }

private:
    class ConcurrentQueue;
    friend class ConcurrentQueue;

    class SerialQueue;
    friend class SerialQueue;

    class MainQueue;
    friend class MainQueue;

    class Group;
    friend class Group;

    class QueueBase;
    friend class QueueBase;

    bool _initialised;

    RefPtr<Log> _log;
    RefPtr<ThreadPool> _threadPool;
    RefPtr<TaskQueue> _concurrentQueue;
    RefPtr<TaskQueue> _mainQueue;
    bool _closeThreadPool;
};
}

#endif
