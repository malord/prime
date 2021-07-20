// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OSX_GCDTASKSYSTEM_H
#define PRIME_OSX_GCDTASKSYSTEM_H

#include "GCDConfig.h"

#ifndef PRIME_NO_GCD

#include "../Log.h"
#include "../TaskSystem.h"
#include "../Thread.h" // Needed for the thread ID, since GCD doesn't tell us if we're the main thread
#include "GCDTaskQueue.h"

namespace Prime {

/// TaskSystem implementation for Grand Central Dispatch.
class PRIME_PUBLIC GCDTaskSystem : public TaskSystem {
public:
    GCDTaskSystem();

    ~GCDTaskSystem();

    /// Assumes the calling thread is the main thread. If not, call reassignMainThread().
    bool init(Log* log);

    /// For compatibility with DefaultTaskSystem.
    bool init(int concurrentThreadCount, int maxThreadCount, size_t stackSize, Log* log)
    {
        (void)concurrentThreadCount;
        (void)maxThreadCount;
        (void)stackSize;
        return init(log);
    }

    void reassignMainThread();

    /// In a Cocoa or UIKit application this should be the default false, since the run loop calls dispatch_main
    /// for us.
    void setRunMainThreadQueueEnabled(bool value) { _runMainThreadQueue = value; }

    // TaskSystem implementation.
    virtual TaskQueue* getConcurrentQueue() PRIME_OVERRIDE { return &_concurrent; }
    virtual TaskQueue* getMainThreadQueue() PRIME_OVERRIDE { return &_main; }
    virtual void runMainThreadQueue() PRIME_OVERRIDE;
    virtual bool isMainThread() PRIME_OVERRIDE;
    virtual RefPtr<TaskQueue> createSerialQueue() PRIME_OVERRIDE;

private:
    GCDTaskQueue _concurrent;
    GCDTaskQueue _main;

    bool _runMainThreadQueue;
    Thread::ThreadID _mainThreadID;
};

}

#endif

#endif
