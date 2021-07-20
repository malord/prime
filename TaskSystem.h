// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TASKSYSTEM_H
#define PRIME_TASKSYSTEM_H

#include "TaskQueue.h"

namespace Prime {

/// A TaskSystem is responsible for coordinating thread usage and providing TaskQueues for an application.
class PRIME_PUBLIC TaskSystem : public RefCounted {
public:
    /// It's up to the application to initialise this - no default is provided.
    static void setGlobal(TaskSystem* taskSystem) { _global = taskSystem; }

    /// Where possible, library code should not use this and should instead take a TaskQueue parameter.
    static TaskSystem* getGlobal() { return _global; }

    TaskSystem() { }

    virtual ~TaskSystem();

    /// Returns a TaskQueue that will run any tasks queued on it in one or more background threads.
    virtual TaskQueue* getConcurrentQueue() = 0;

    /// Returns a TaskQueue that runs tasks sequentially on the application's main thread. This is a useful
    /// mechanism for synchronising the application with tasks (e.g., when a task completes it can queue a task on
    /// the main thread to update the user interface).
    virtual TaskQueue* getMainThreadQueue() = 0;

    RefPtr<TaskQueue::TaskGroup> createTaskGroup()
    {
        return getConcurrentQueue()->createTaskGroup();
    }

    /// Call this from the main thread to ensure tasks queued on the main thread queue are run.
    virtual void runMainThreadQueue() = 0;

    /// Return true if this is the main thread.
    virtual bool isMainThread() = 0;

    /// Create a new serial queue (one which runs the next task only when the previous task has completed). Tasks
    /// are still run in a background thread. Useful for queueing I/O for slow media (e.g., Blu-rays).
    virtual RefPtr<TaskQueue> createSerialQueue() = 0;

    /// Tell TaskSystem that the calling thread is about to wait on something and won't be using the CPU, allowing
    /// another thread to run. If this method returns true then you _must_ call resumeDoNotCallDirectly(). If it
    /// returns false, you _must not_ call resumeDoNotCallDirectly(). Use the ScopedYield class (Common.h)
    /// instead of calling this method directly.
    virtual bool yieldDoNotCallDirectly();

    /// Tell TaskSystem that the calling thread is no longer waiting for I/O and would like to resume. You must
    /// call this for a corresponding call to yieldDoNotCallDirectly() where yieldDoNotCallDirectly() returned
    /// true. Use the ScopedYield class (Common.h) instead of calling this method directly.
    virtual void resumeDoNotCallDirectly();

private:
    static TaskSystem* _global;

    PRIME_UNCOPYABLE(TaskSystem);
};
}

#endif
