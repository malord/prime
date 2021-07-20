// Copyright 2000-2021 Mark H. P. Lord

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include "GCDTaskSystem.h"
#include "GCDTaskQueue.h"

#ifndef PRIME_NO_GCD

namespace Prime {

GCDTaskSystem::GCDTaskSystem()
    : _runMainThreadQueue(false)
    , _mainThreadID(Thread::getCallingThreadID())
{
}

GCDTaskSystem::~GCDTaskSystem()
{
}

bool GCDTaskSystem::init(Log* log)
{
    (void)log; // In case we report errors in the future

    _concurrent.setDispatchQueue(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
    _main.setDispatchQueue(dispatch_get_main_queue());

    reassignMainThread();

    return true;
}

void GCDTaskSystem::reassignMainThread()
{
    _mainThreadID = Thread::getCallingThreadID();
}

void GCDTaskSystem::runMainThreadQueue()
{
    if (!_runMainThreadQueue) {
        dispatch_main();
    }
}

bool GCDTaskSystem::isMainThread()
{
    return Thread::getCallingThreadID() == _mainThreadID;
}

RefPtr<TaskQueue> GCDTaskSystem::createSerialQueue()
{
    dispatch_queue_t queue = dispatch_queue_create(NULL, NULL);
    if (!queue) {
        return NULL;
    }

    RefPtr<GCDTaskQueue> newTaskQueue = PassRef(new GCDTaskQueue(queue));

    dispatch_release(queue);

    return newTaskQueue;
}
}

#endif
