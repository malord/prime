// Copyright 2000-2021 Mark H. P. Lord

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include "GCDTaskQueue.h"

#ifndef PRIME_NO_GCD

#include "../Event.h"
#include "../Mutex.h"

namespace Prime {

//
// GCDTaskQueue::GCDTaskGroup
//

GCDTaskQueue::GCDTaskGroup::GCDTaskGroup()
    : _group(NULL)
{
}

GCDTaskQueue::GCDTaskGroup::GCDTaskGroup(dispatch_group_t group)
    : _group(group)
{
    if (group) {
        dispatch_retain(group);
    }
}

GCDTaskQueue::GCDTaskGroup::~GCDTaskGroup()
{
    if (_group) {
        dispatch_release(_group);
    }
}

bool GCDTaskQueue::GCDTaskGroup::create()
{
    if (_group) {
        dispatch_release(_group);
        _group = NULL;
    }

    dispatch_group_t group = dispatch_group_create();
    if (!group) {
        return false;
    }

    _group = group;
    return true;
}

void GCDTaskQueue::GCDTaskGroup::setGroup(dispatch_group_t group)
{
    if (group) {
        dispatch_retain(group);
    }

    if (_group) {
        dispatch_release(_group);
    }

    _group = group;
}

void GCDTaskQueue::GCDTaskGroup::queue(TaskQueue* queue, const Callback& callbackReference)
{
    PRIME_ASSERT(queue);
    PRIME_ASSERT(_group);

    Callback callback = callbackReference;

    dispatch_group_async(_group, static_cast<GCDTaskQueue*>(queue)->getDispatchQueue(), ^{
        callback();
    });
}

void GCDTaskQueue::GCDTaskGroup::wait()
{
    if (_group) {
        dispatch_group_wait(_group, DISPATCH_TIME_FOREVER);
    }
}

//
// GCDTaskQueue
//

GCDTaskQueue::GCDTaskQueue()
    : _queue(NULL)
{
}

GCDTaskQueue::GCDTaskQueue(dispatch_queue_t queue)
    : _queue(queue)
{
    if (queue) {
        dispatch_retain(queue);
    }
}

GCDTaskQueue::~GCDTaskQueue()
{
    if (_queue) {
        dispatch_release(_queue);
    }
}

void GCDTaskQueue::setDispatchQueue(dispatch_queue_t queue)
{
    if (queue) {
        dispatch_retain(queue);
    }

    if (_queue) {
        dispatch_release(_queue);
    }

    _queue = queue;
}

void GCDTaskQueue::queue(const Callback& callbackReference)
{
    PRIME_ASSERT(_queue);

    Callback callback = callbackReference;

    dispatch_async(_queue, ^{
        callback();
    });
}

void GCDTaskQueue::apply(const ApplyCallback& callbackReference, size_t count)
{
    PRIME_ASSERT(_queue);

    ApplyCallback callback = callbackReference;

    dispatch_apply(count, _queue, ^(size_t iteration) {
        callback(iteration);
    });
}

void GCDTaskQueue::queueApply(const ApplyCallback& callbackReference, size_t count,
    const Callback& finishCallbackReference)
{
    PRIME_ASSERT(_queue);

    ApplyCallback callback = callbackReference;
    Callback finishCallback = finishCallbackReference;

    dispatch_async(_queue, ^{
        dispatch_apply(count, _queue, ^(size_t iteration) {
            callback(iteration);
        });

        finishCallback();
    });
}

void GCDTaskQueue::queueAndWait(const Callback& callbackReference)
{
    PRIME_ASSERT(_queue);

    Callback callback = callbackReference;

    dispatch_sync(_queue, ^{
        callback();
    });
}

RefPtr<TaskQueue::TaskGroup> GCDTaskQueue::createTaskGroup()
{
    RefPtr<GCDTaskGroup> newTaskGroup = PassRef(new GCDTaskGroup);

    if (!newTaskGroup->create()) {
        return NULL;
    }

    return newTaskGroup;
}

}

#endif
