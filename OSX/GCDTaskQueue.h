// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OSX_GCDTASKQUEUE_H
#define PRIME_OSX_GCDTASKQUEUE_H

#include "GCDConfig.h"

#ifndef PRIME_NO_GCD

#include "../TaskQueue.h"

namespace Prime {

/// A TaskQueue implementation for Grand Central Dispatch.
class PRIME_PUBLIC GCDTaskQueue : public TaskQueue {
public:
    GCDTaskQueue();

    explicit GCDTaskQueue(dispatch_queue_t queue);

    ~GCDTaskQueue();

    void setDispatchQueue(dispatch_queue_t queue);

    dispatch_queue_t getDispatchQueue() const { return _queue; }

    // TaskQueue implementation.
    virtual void queue(const Callback& callback) PRIME_OVERRIDE;
    virtual void apply(const ApplyCallback& callback, size_t count) PRIME_OVERRIDE;
    virtual void queueApply(const ApplyCallback& callback, size_t count,
        const Callback& finishCallback) PRIME_OVERRIDE;
    virtual void queueAndWait(const Callback& callback) PRIME_OVERRIDE;
    virtual RefPtr<TaskGroup> createTaskGroup() PRIME_OVERRIDE;

    /// TaskGroup implementation for Grand Central Dispatch.
    class PRIME_PUBLIC GCDTaskGroup : public TaskGroup {
    public:
        GCDTaskGroup();

        explicit GCDTaskGroup(dispatch_group_t group);

        ~GCDTaskGroup();

        void setGroup(dispatch_group_t group);

        bool create();

        // TaskGroup implementation.
        virtual void queue(TaskQueue* queue, const Callback& callback) PRIME_OVERRIDE;
        virtual void wait() PRIME_OVERRIDE;

    private:
        dispatch_group_t _group;

        PRIME_UNCOPYABLE(GCDTaskGroup);
    };

private:
    dispatch_queue_t _queue;

    PRIME_UNCOPYABLE(GCDTaskQueue);
};

}

#endif

#endif
