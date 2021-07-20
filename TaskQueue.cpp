// Copyright 2000-2021 Mark H. P. Lord

#include "TaskQueue.h"

namespace Prime {

//
// TaskQueue
//

TaskQueue::TaskQueue()
{
}

TaskQueue::~TaskQueue()
{
}

TaskQueue::Waitable TaskQueue::queueWaitable(const Callback& callback)
{
    return Waitable(this, callback);
}

bool TaskQueue::yieldDoNotCallDirectly()
{
    return false;
}

void TaskQueue::resumeDoNotCallDirectly()
{
    PRIME_ASSERTMSG(0, "resume() without yield()");
}

//
// TaskQueue::TaskGroup
//

TaskQueue::TaskGroup::TaskGroup()
{
}

TaskQueue::TaskGroup::~TaskGroup()
{
}

//
// TaskQueue::Waitable
//

TaskQueue::Waitable::Waitable(TaskQueue* queue, const Callback& callback)
{
    _data = PassRef(new Data(callback));
#ifdef PRIME_CXX11_STL
    auto data = Ref(_data);
    queue->queue([data] { data->run(); });
#else
    queue->queue(MethodCallback(Ref(_data), &Data::run));
#endif
}

//
// TaskQueue::Waitable::Data
//

TaskQueue::Waitable::Data::Data(const Callback& callback)
    : _callback(callback)
{
    _sem.init(0, Log::getGlobal());
}

void TaskQueue::Waitable::Data::wait()
{
    if (_sem.isInitialised()) {
        _sem.lock();
    }
}

bool TaskQueue::Waitable::Data::tryWait()
{
    if (_sem.isInitialised()) {
        return _sem.tryLock();
    }

    return true;
}

bool TaskQueue::Waitable::Data::tryWait(int milliseconds)
{
    if (_sem.isInitialised()) {
        return _sem.tryLock(milliseconds);
    }

    return true;
}

void TaskQueue::Waitable::Data::run()
{
    if (_sem.isInitialised()) {
        _callback();
        _sem.unlock();
    }
}
}
