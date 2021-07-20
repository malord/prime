// Copyright 2000-2021 Mark H. P. Lord

#include "ThreadPoolTaskSystem.h"
#include "Callback.h" // TODO: inefficient pre-C++11, where callbacks are std::functions
#include "Clocks.h"
#include "NumberUtils.h"
#include "ScopedPtr.h"
#include "Semaphore.h"

// Make the first thread do some work
#define APPLY_SHOULD_RUN_FIRST_BATCH

// Not doing so can cause severe stack overflows in some cases
// Usually means the code needs to be re-thought, however
#define APPLY_SHOULD_YIELD

namespace Prime {

//
// ThreadPoolTaskSystem::QueueBase
//

class ThreadPoolTaskSystem::QueueBase : public TaskQueue {
public:
    // TaskQueue implementation.
    virtual void apply(const ApplyCallback& callback, size_t count) PRIME_OVERRIDE;
    virtual void queueApply(const ApplyCallback& callback, size_t count,
        const Callback& finishCallback) PRIME_OVERRIDE;
    virtual RefPtr<TaskGroup> createTaskGroup() PRIME_OVERRIDE;
    virtual bool yieldDoNotCallDirectly() PRIME_OVERRIDE;
    virtual void resumeDoNotCallDirectly() PRIME_OVERRIDE;

protected:
    void init(ThreadPool* threadPool, Log* log);

    RefPtr<Log> _log;
    RefPtr<ThreadPool> _threadPool;
};

//
// ThreadPoolTaskSystem::ConcurrentQueue
//

class ThreadPoolTaskSystem::ConcurrentQueue : public ThreadPoolTaskSystem::QueueBase {
public:
    ConcurrentQueue();

    bool init(ThreadPool* threadPool, Log* log);

    // TaskQueue implementation.
    virtual void queue(const Callback& callback) PRIME_OVERRIDE;
    virtual void queueAndWait(const Callback& callback) PRIME_OVERRIDE;
    virtual void apply(const ApplyCallback& callback, size_t count) PRIME_OVERRIDE;
    virtual void queueApply(const ApplyCallback& callback, size_t count,
        const Callback& finishCallback) PRIME_OVERRIDE;
};

ThreadPoolTaskSystem::ConcurrentQueue::ConcurrentQueue()
{
}

bool ThreadPoolTaskSystem::ConcurrentQueue::init(ThreadPool* threadPool, Log* log)
{
    QueueBase::init(threadPool, log);

    return true;
}

void ThreadPoolTaskSystem::ConcurrentQueue::queue(const Callback& callback)
{
    _threadPool->queue(callback);
}

void ThreadPoolTaskSystem::ConcurrentQueue::queueAndWait(const Callback& callback)
{
    if (_threadPool->isCallingThreadInPool()) {
        // We're running on one of our ThreadPool's threads. Since we're concurrent, and can run callbacks in any
        // order we like, just invoke the callback ourselves so that the calling thread can finish sooner.
        callback();

    } else {
        queueWaitable(callback).wait();
    }
}

namespace {
    enum { maxBatches = 128u };

    struct ConcurrentApplyBatch : public RefCounted {
        TaskQueue::ApplyCallback _callback;
        size_t _index;
        size_t _end;
        AtomicCounter* _refCounter;
        TimedSemaphore* _sem;
        AtomicCounter _runCounter;

        ConcurrentApplyBatch()
            : _runCounter(1)
        {
        }

        void run()
        {
            if (!runIfNotRunning()) {
                // Trace("Task already run!");
            }
        }

        bool runIfNotRunning()
        {
            if (_runCounter.decrement() != 0) {
                return false;
            }

            for (; _index < _end; ++_index) {
                _callback(_index);
            }

            // It is imperative that there's no more data access after this.
            if (_refCounter->decrement() == 0) {
                _sem->unlock();
            }

            _refCounter = NULL;
            _sem = NULL;

            return true;
        }

        static void fillBatches(RefPtr<ConcurrentApplyBatch>* batches, const TaskQueue::ApplyCallback& callback,
            size_t count, size_t batchCount, AtomicCounter* refCounter, TimedSemaphore* sem)
        {
            size_t perBatch = count / batchCount;
            size_t extra = count % batchCount;

            for (size_t i = 0; i != batchCount; ++i) {
                RefPtr<ConcurrentApplyBatch>& batch = batches[i];
                batch = PassRef(new ConcurrentApplyBatch);
                batch->_callback = callback;
                batch->_index = (i == 0) ? 0 : batches[i - 1]->_end;
                size_t perThisBatch = perBatch + ((i < extra) ? 1 : 0);
                batch->_end = batch->_index + perThisBatch;
                batch->_refCounter = refCounter;
                batch->_sem = sem;
            }

            PRIME_ASSERT(batches[batchCount - 1]->_end == count);
        }
    };
}

void ThreadPoolTaskSystem::ConcurrentQueue::apply(const ApplyCallback& callback, size_t count)
{
    size_t i; // For VC6

    if (count < 1) {
        return;
    }

    // Allocating an object for each batch gives us flexibility in how the batches are run.

    RefPtr<ConcurrentApplyBatch> batches[maxBatches];

    unsigned int batchCount = (unsigned int)Min((size_t)maxBatches, count);

    AtomicCounter refCounter(batchCount);

    // We have to use a semaphore with a count of 1 because a pthreads non-recursive-mutex can't be unlocked
    // (in a portable way) by a thread that didn't lock it.
    TimedSemaphore sem;

    if (!sem.init(0, _log, "Concurrent dispatch queue apply semaphore")) {
        for (i = 0; i != count; ++i) {
            callback(i);
        }

        return;
    }

    ConcurrentApplyBatch::fillBatches(batches, callback, count, batchCount, &refCounter, &sem);

    ThreadPool::ScopedEnter enter(_threadPool);

    // We want to use this thread to run real work, but we don't want to run another queued task which also
    // calls apply(), which runs another queued task which also calls apply(), etc. So, only run tasks on
    // this thread if we're not already running an apply.
    if (enter.getCount() == 1) {
        // First time this thread has been re-entered. Run one of the batches ourselves.

        // Run the first batch ourselves.
        for (i = 1; i != batchCount; ++i) {
            _threadPool->queue(MethodCallback(Ref(batches[i]), &ConcurrentApplyBatch::run));
        }

        batches[0]->run();

        // Run through the batches backwards and run any that haven't been run yet by the thread pool.
        for (i = batchCount - 1; i > 0; --i) {
            if (!batches[i]->runIfNotRunning()) {
                //Trace("Queue got that one!");
            }
        }

        // Make sure the application doesn't deadlock due to being down a thread due to an inordinately time
        // consuming batch.
        if (!sem.tryLock(500)) {
            ThreadPool::ScopedAddThread addThread(_threadPool);
            sem.lock();
        }

    } else {
        // Either there's already another call to apply() somewhere in the call stack, or this isn't a ThreadPool
        // thread. This thread can't be used to run tasks, so queue the tasks then spawn another thread while
        // we wait.
        for (i = 0; i != batchCount; ++i) {
            _threadPool->queue(MethodCallback(Ref(batches[i]), &ConcurrentApplyBatch::run));
        }

        ThreadPool::ScopedAddThread addThread(_threadPool);
        sem.lock();
    }
}

namespace {
    struct ConcurrentApplyTask : public RefCounted {
        TaskQueue::ApplyCallback callback;
        size_t count;
        TaskQueue::Callback finishCallback;
        RefPtr<TaskQueue> queue;

        void run()
        {
            queue->apply(callback, count);
            finishCallback();
        }
    };
}

void ThreadPoolTaskSystem::ConcurrentQueue::queueApply(const ApplyCallback& callback, size_t count,
    const Callback& finishCallback)
{
    RefPtr<ConcurrentApplyTask> task = PassRef(new ConcurrentApplyTask);
    task->callback = callback;
    task->count = count;
    task->finishCallback = finishCallback;
    task->queue = this;

    queue(MethodCallback(Ref(task), &ConcurrentApplyTask::run));
}

//
// ThreadPoolTaskSystem::SerialQueue
//

class ThreadPoolTaskSystem::SerialQueue : public ThreadPoolTaskSystem::QueueBase {
public:
    SerialQueue();

    bool init(ThreadPool* threadPool, Log* log);

    // TaskQueue implementation.
    virtual void queue(const Callback& callback) PRIME_OVERRIDE;
    virtual void queueAndWait(const Callback& callback) PRIME_OVERRIDE;

    // Made public for VC6.
    void runOne();

private:
    RecursiveMutex _mutex;
    std::list<Callback> _queue;
    bool _queuedInThreadPool;
};

ThreadPoolTaskSystem::SerialQueue::SerialQueue()
{
}

bool ThreadPoolTaskSystem::SerialQueue::init(ThreadPool* threadPool, Log* log)
{
    QueueBase::init(threadPool, log);
    _queuedInThreadPool = false;

    if (!_mutex.init(_log, "Serial dispatch queue mutex")) {
        return false;
    }

    return true;
}

void ThreadPoolTaskSystem::SerialQueue::queue(const Callback& callback)
{
    RecursiveMutex::ScopedLock lock(&_mutex);

    _queue.push_back(callback);

    if (!_queuedInThreadPool) {
        _queuedInThreadPool = true;
        retain(); // Don't let us be destructed while a thread is running.
        _threadPool->queue(MethodCallback(this, &ThreadPoolTaskSystem::SerialQueue::runOne));
    }
}

void ThreadPoolTaskSystem::SerialQueue::runOne()
{
    Callback callback;
    {
        RecursiveMutex::ScopedLock lock(&_mutex);
        if (!PRIME_GUARDMSG(!_queue.empty(), "We shouldn't have been queued if there are no tasks.")) {
            _queuedInThreadPool = false;
            release();
            return;
        }

        callback = PRIME_MOVE(_queue.front());
        _queue.pop_front();
    }

    callback();

    {
        RecursiveMutex::ScopedLock lock(&_mutex);
        if (_queue.empty()) {
            _queuedInThreadPool = false; // This can't be set to false until the callback has finished!
            release();
        } else {
            // Queue us again so we run our next callback in turn.
            _threadPool->queue(MethodCallback(this, &ThreadPoolTaskSystem::SerialQueue::runOne));
        }
    }
}

void ThreadPoolTaskSystem::SerialQueue::queueAndWait(const Callback& callback)
{
    if (_threadPool->isCallingThreadInPool()) {
        // We're running on one of our ThreadPool's threads. Since we'd deadlock otherwise, run it now.
        callback();

    } else {
        queueWaitable(callback).wait();
    }
}

//
// ThreadPoolTaskSystem::MainQueue
//

class ThreadPoolTaskSystem::MainQueue : public ThreadPoolTaskSystem::QueueBase {
public:
    MainQueue();

    bool init(ThreadPool* threadPool, Log* log);

    void run();

    void setTaskQueuedCallback(const Callback& value);

    bool isMainThread() const { return Thread::getCallingThreadID() == _mainThreadID; }

    // TaskQueue implementation.
    virtual void queue(const Callback& callback) PRIME_OVERRIDE;
    virtual void queueAndWait(const Callback& callback) PRIME_OVERRIDE;

private:
    RecursiveMutex _mutex;
    std::list<Callback> _queue;
    Callback _taskQueuedCallback;
    Thread::ThreadID _mainThreadID;
};

ThreadPoolTaskSystem::MainQueue::MainQueue()
    : _mainThreadID(Thread::getCallingThreadID())
{
}

bool ThreadPoolTaskSystem::MainQueue::init(ThreadPool* threadPool, Log* log)
{
    QueueBase::init(threadPool, log);

    if (!_mutex.init(_log, "Main dispatch queue mutex")) {
        return false;
    }

    _mainThreadID = Thread::getCallingThreadID();

    return true;
}

void ThreadPoolTaskSystem::MainQueue::run()
{
    if (Thread::getCallingThreadID() != _mainThreadID) {
        _log->developerWarning("Main thread queue running on not the main thread.");
    }

    for (;;) {
        Callback callback;
        {
            RecursiveMutex::ScopedLock lock(&_mutex);

            if (_queue.empty()) {
                break;
            }

            callback = PRIME_MOVE(_queue.front());
            _queue.pop_front();
        }

        callback();
    }
}

void ThreadPoolTaskSystem::MainQueue::setTaskQueuedCallback(const Callback& value)
{
    RecursiveMutex::ScopedLock lock(&_mutex);
    _taskQueuedCallback = value;
}

void ThreadPoolTaskSystem::MainQueue::queue(const Callback& callback)
{
    RecursiveMutex::ScopedLock lock(&_mutex);

    if (_queue.empty() && _taskQueuedCallback) {
        _taskQueuedCallback();
    }

    _queue.push_back(callback);
}

void ThreadPoolTaskSystem::MainQueue::queueAndWait(const Callback& callback)
{
    if (isMainThread()) {
        callback();
        return;
    }

    ThreadPool::ScopedAddThread addThread(NULL);
    if (_threadPool->isCallingThreadInPool()) {
        addThread.addThread(_threadPool);
    }
    queueWaitable(callback).wait();
}

//
// ThreadPoolTaskSystem::Group
//

class ThreadPoolTaskSystem::Group : public TaskQueue::TaskGroup {
public:
    Group();

    bool init(ThreadPool* threadPool, Log* log);

    // TaskGroup implementation.
    virtual void queue(TaskQueue* queue, const TaskQueue::Callback& callback) PRIME_OVERRIDE;
    virtual void wait() PRIME_OVERRIDE;

private:
    int _counter;
    RefPtr<ThreadPool> _threadPool;
    RefPtr<Log> _log;
    RecursiveMutex _mutex;
    Condition _allFinished;

    class QueueItem {
    public:
        QueueItem(const TaskQueue::Callback& callback, Group* group)
            : _callback(callback)
            , _group(group)
        {
        }

        void queue(TaskQueue* taskQueue) { taskQueue->queue(MethodCallback(this, &QueueItem::run)); }

    private:
        TaskQueue::Callback _callback;
        Group* _group;

        void run()
        {
            _callback();
            _group->queueItemFinished();
            delete this;
        }

        PRIME_UNCOPYABLE(QueueItem);
    };

    friend class QueueItem;

    void queueItemFinished();
};

ThreadPoolTaskSystem::Group::Group()
{
}

bool ThreadPoolTaskSystem::Group::init(ThreadPool* threadPool, Log* log)
{
    _log = log;
    _threadPool = threadPool;
    _counter = 0;

    if (!_mutex.init(_log, "Dispatch group mutex")) {
        return false;
    }

    if (!_allFinished.init(_log, "Dispatch group condition")) {
        return false;
    }

    return true;
}

void ThreadPoolTaskSystem::Group::queue(TaskQueue* queue, const TaskQueue::Callback& callback)
{
    RecursiveMutex::ScopedLock lock(&_mutex);

    // TODO: pooled/batched allocation
    (new QueueItem(callback, this))->queue(queue);

    if (_counter++ == 0) {
        retain();
    }
}

void ThreadPoolTaskSystem::Group::queueItemFinished()
{
    RecursiveMutex::ScopedLock lock(&_mutex);

    if (--_counter > 0) {
        return;
    }

    _allFinished.wakeAll();
    lock.unlock();
    release();
}

void ThreadPoolTaskSystem::Group::wait()
{
    // If we're a thread pool thread then wake up another thread while we're waiting.
    bool addedThread = _threadPool->isCallingThreadInPool() && _threadPool->addThread();

    {
        RecursiveMutex::ScopedLock lock(&_mutex);

        while (_counter != 0) {
            _allFinished.wait(lock);
        }
    }

    if (addedThread) {
        _threadPool->removeThread();
    }
}

//
// ThreadPoolTaskSystem::QueueBase implementation
//

void ThreadPoolTaskSystem::QueueBase::init(ThreadPool* threadPool, Log* log)
{
    _log = log;
    _threadPool = threadPool;
}

namespace {
    struct ApplyItem {
        size_t index;
        TaskQueue::ApplyCallback callback;

        void run()
        {
            callback(index);
        }
    };
}

void ThreadPoolTaskSystem::QueueBase::apply(const ApplyCallback& callback, size_t count)
{
    ApplyItem item;

    item.callback = callback;
    for (item.index = 0; item.index != count; ++item.index) {
        queueAndWait(MethodCallback(&item, &ApplyItem::run));
    }
}

namespace {
    struct SerialApplyTask : public RefCounted {
        TaskQueue::ApplyCallback callback;
        size_t count;
        TaskQueue::Callback finishCallback;

        void run()
        {
            for (size_t index = 0; index != count; ++index) {
                callback(index);
            }

            callback = TaskQueue::ApplyCallback();
            finishCallback();
        }
    };
}

void ThreadPoolTaskSystem::QueueBase::queueApply(const ApplyCallback& callback, size_t count,
    const Callback& finishCallback)
{
    RefPtr<SerialApplyTask> task = PassRef(new SerialApplyTask);
    task->callback = callback;
    task->count = count;
    task->finishCallback = finishCallback;

    queue(MethodCallback(Ref(task), &SerialApplyTask::run));
}

RefPtr<TaskQueue::TaskGroup> ThreadPoolTaskSystem::QueueBase::createTaskGroup()
{
    RefPtr<Group> group = PassRef(new Group);
    if (!group->init(_threadPool, _log)) {
        return NULL;
    }

    return group;
}

bool ThreadPoolTaskSystem::QueueBase::yieldDoNotCallDirectly()
{
    if (!_threadPool) {
        return false;
    }

    PRIME_DEBUG_ASSERT(_threadPool->isCallingThreadInPool());

    return _threadPool->addThread();
}

void ThreadPoolTaskSystem::QueueBase::resumeDoNotCallDirectly()
{
    PRIME_ASSERT(_threadPool);
    _threadPool->removeThread();
}

//
// ThreadPoolTaskSystem
//

ThreadPoolTaskSystem::ThreadPoolTaskSystem()
{
    _initialised = false;
    _closeThreadPool = false;
}

ThreadPoolTaskSystem::~ThreadPoolTaskSystem()
{
    close();
}

bool ThreadPoolTaskSystem::init(int concurrentThreadCount, int maxThreadCount, size_t stackSize, Log* log)
{
    _threadPool = PassRef(new ThreadPool);
    if (!_threadPool->init(concurrentThreadCount, maxThreadCount, stackSize, log)) {
        _threadPool.release();
        return false;
    }

    if (!ThreadPoolTaskSystem::init(_threadPool, log)) {
        return false;
    }

    _closeThreadPool = true;
    return true;
}

bool ThreadPoolTaskSystem::init(ThreadPool* threadPool, Log* log)
{
    if (_initialised) {
        return true;
    }

    _log = log;
    _threadPool = threadPool;

    RefPtr<ConcurrentQueue> concurrentQueue = PassRef(new ConcurrentQueue);
    if (!concurrentQueue->init(_threadPool, _log)) {
        close();
        return false;
    }
    _concurrentQueue = concurrentQueue;

    RefPtr<MainQueue> mainQueue = PassRef(new MainQueue);
    if (!mainQueue->init(_threadPool, _log)) {
        close();
        return false;
    }
    _mainQueue = mainQueue;

    _initialised = true;

    return true;
}

void ThreadPoolTaskSystem::setMainQueueTaskQueuedCallback(const TaskQueue::Callback& value)
{
    if (PRIME_GUARD(_mainQueue.get())) {
        static_cast<MainQueue*>(_mainQueue.get())->setTaskQueuedCallback(value);
    }
}

void ThreadPoolTaskSystem::close()
{
    _mainQueue.release();
    _concurrentQueue.release();
    if (_threadPool && _closeThreadPool) {
        _threadPool->close();
    }
    _threadPool.release();
    _log.release();
    _initialised = false;
}

void ThreadPoolTaskSystem::runMainThreadQueue()
{
    static_cast<MainQueue*>(_mainQueue.get())->run();
}

bool ThreadPoolTaskSystem::isMainThread()
{
    return static_cast<MainQueue*>(_mainQueue.get())->isMainThread();
}

RefPtr<TaskQueue> ThreadPoolTaskSystem::createSerialQueue()
{
    RefPtr<SerialQueue> queue = PassRef(new SerialQueue);
    if (!queue->init(_threadPool, _log)) {
        return NULL;
    }

    return queue;
}

bool ThreadPoolTaskSystem::yieldDoNotCallDirectly()
{
    if (!_threadPool) {
        return false;
    }

    return _threadPool->isCallingThreadInPool() && _threadPool->addThread();
}

void ThreadPoolTaskSystem::resumeDoNotCallDirectly()
{
    PRIME_ASSERT(_threadPool);
    _threadPool->removeThread();
}
}
