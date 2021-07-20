// Copyright 2000-2021 Mark H. P. Lord

#include "ThreadPool.h"
#include "Timeout.h"
//#include "Spew.h" // Uncomment to diagnose ThreadPool behaviour
//#include "Spew2.h" // For even more detail
//#include "CPUUsage.h"
#include <algorithm>

namespace Prime {

ThreadSpecificData* ThreadPool::getThreadSpecificData()
{
    static ThreadSpecificData tsd(Log::getGlobal(), NULL, "Thread pool thread-specific data");
    return &tsd;
}

ThreadPool::ThreadPool()
    : _initialised(false)
{
}

ThreadPool::~ThreadPool()
{
    close();
}

const char* ThreadPool::getName() const
{
    return _name.c_str();
}

bool ThreadPool::init(int maxConcurrent, int maxThreads, size_t stackSize, Log* log, const char* debugName)
{
    PRIME_ASSERT(!isInitialised());

    if (maxConcurrent <= 0) {
        int cpuCount = Thread::getCPUCount(log);
        if (cpuCount < 1) {
            return false;
        }

        if (maxConcurrent < 0) {
            maxConcurrent = -maxConcurrent * cpuCount;
        } else {
            maxConcurrent = cpuCount;
        }
    }

    _log = log;
    _name = debugName;
    _timeoutMilliseconds = 10000; // TODO: this should be an option
    _maxThreads = maxThreads >= 1 ? maxThreads : defaultMaxThreads;
    _stackSize = stackSize ? stackSize : defaultStackSize;

    // Do this before any threads ask for it.
    if (!getThreadSpecificData()) {
        log->error(PRIME_LOCALISE("Unable to create thread pool thread-specific-data."));
        return false;
    }

    _threadCount = 0;
    _numberOfThreadsThatShouldExit = 0;

    log->trace("%s: %d concurrent thread(s) maximum, %d thread(s) limit.", getName(), maxConcurrent, maxThreads);

    _initialised = true; // If we fail from now on, close() will be called.

    // The Mutex needs to be recursive.
    if (!_mutex.init(log, "Thread pool mutex") || !_taskQueued.init(&_mutex, log, "Thread pool task queued") || !_threadExited.init(&_mutex, log, "Thread pool thread exit") || !_concurrentSemaphore.init(0, log, "Thread pool concurrent semaphore")) {

        close();
        return false;
    }

    _concurrentSemaphore.post(maxConcurrent);

    return true;
}

bool ThreadPool::addThread()
{
    RecursiveMutex::ScopedLock lock(&_mutex);

    if (_numberOfThreadsThatShouldExit > 0) {
        // Another thread is waiting for a thread to exit. We'll be that thread.

        --_numberOfThreadsThatShouldExit;

        PRIME_SPEW("addThread: doing nothing due to exit request: now %d should exit.", _numberOfThreadsThatShouldExit);
        _threadExited.wakeOne(); // We exited.

    } else if (_maxThreads && _threadCount - _numberOfThreadsThatShouldExit >= _maxThreads) {
        PRIME_SPEW2("addThread: not adding a thread, limit of %d thread(s) reached.", _maxThreads);
        return false;

    } else if (!_tasks.empty()) {
        // There are tasks waiting, so immediately create a new thread.
        PRIME_SPEW2("addThread: creating thread.");
        createThread(lock); // If this fails, it posts the semaphore.

    } else {
        // There are no tasks waiting. Up the semaphore so that a thread is created the next time a task is queued.
        PRIME_SPEW2("addThread: posting semaphore.");
        _concurrentSemaphore.post(1);
    }

    return true;
}

bool ThreadPool::createThread(RecursiveMutex::ScopedLock&)
{
#ifdef PRIME_CXX11_STL
    if (!Thread().create([this]() { this->thread(); }, _stackSize, _log, getName())) {
#else
    if (!Thread().create(MethodCallback(this, &ThreadPool::thread), _stackSize, _log, getName())) {
#endif
        _concurrentSemaphore.post(1);
        return false;
    }

    ++_threadCount;
    PRIME_SPEW("%s: %d thread(s) running, %d should exit.", getName(), _threadCount, _numberOfThreadsThatShouldExit);

    return true;
}

void ThreadPool::removeThread()
{
    PRIME_ASSERT(_threadCount > 0);

    RecursiveMutex::ScopedLock lock(&_mutex);

    if (!_concurrentSemaphore.tryLock()) {
        // We're not able to prevent another thread launching, so we have to wait for one to exit.
        ++_numberOfThreadsThatShouldExit;
        PRIME_SPEW("removeThread: waking thread. %d should exit.", _numberOfThreadsThatShouldExit);
        _taskQueued.wakeOne();
        _threadExited.wait(lock);

    } else {
        // We've stopped a thread from launching.
        PRIME_SPEW2("removeThread: acquired semaphore.");
    }
}

void ThreadPool::close()
{
    if (_initialised) {
        int peakThreadCount = -1;

        for (;;) {
            RecursiveMutex::ScopedLock lock(&_mutex);

            if (_threadCount > peakThreadCount) {
                peakThreadCount = _threadCount;
                _log->trace("%s: Waiting for %d thread(s).", getName(), _threadCount);
            }

            if (!_threadCount) {
                run(true, false);

                if (_tasks.empty()) {
                    break;
                }
            } else {
                _numberOfThreadsThatShouldExit = _threadCount;
                _concurrentSemaphore.post(1);
                _taskQueued.wakeOne();
                _threadExited.wait(lock);
            }
        }

        _log->trace("%s: Shutdown complete.", getName());
    }

    _mutex.close();
    _concurrentSemaphore.close();
    _taskQueued.close();
    _threadExited.close();
    _log.release();

    _name.resize(0);

    _initialised = false;
}

void ThreadPool::internalQueue(const Callback& callback, RecursiveMutex::ScopedLock& lock)
{
    PRIME_SPEW2("Queueing task.");

    _tasks.push_back(callback);

    if (_concurrentSemaphore.tryLock()) {
        // We're not yet running with as many threads as we could, so create one.
        createThread(lock); // If this fails, it posts the semaphore, undoing our lock.
    }
}

void ThreadPool::queue(const Callback& callback)
{
    if (!PRIME_GUARD(isInitialised())) {
        return;
    }

    RecursiveMutex::ScopedLock lock(&_mutex);

    internalQueue(callback, lock);
    _taskQueued.wakeOne();
}

void ThreadPool::thread()
{
    ThreadData data = { this, 0 };
    getThreadSpecificData()->set(&data);

    runThread(true, true, true);

    {
        RecursiveMutex::ScopedLock lock(&_mutex);

        --_threadCount;
        PRIME_SPEW("%s: %d thread(s) running, %d should exit.", getName(), _threadCount, _numberOfThreadsThatShouldExit);

        _threadExited.wakeOne();
    }

    getThreadSpecificData()->set(NULL);
}

bool ThreadPool::runThread(bool shouldRunAllTasks, bool shouldWaitForTasks, bool isThread)
{
    bool haveRunAnyTasks = false;

    // We start out owning one concurrentSemaphore lock.

    for (;;) {
        Task task;
        for (;;) {
            RecursiveMutex::ScopedLock lock(&_mutex);

            if (!isThread) {
                if (!shouldRunAllTasks && haveRunAnyTasks) {
                    return !_tasks.empty();
                }

            } else if (_numberOfThreadsThatShouldExit > 0) {
                --_numberOfThreadsThatShouldExit; // Note that _concurrentSemaphore is not increased.
                return !_tasks.empty();
            }

            if (!_tasks.empty()) {
                task = PRIME_MOVE(_tasks.front());
                _tasks.pop_front();
                break;
            }

            if (!isThread) {
                if (!shouldWaitForTasks) {
                    return false;
                }

                _taskQueued.wait(lock);

            } else {
                if (!_taskQueued.timedWait(lock, _timeoutMilliseconds)) {
                    // No tasks within the timeout. This thread isn't needed any more.
                    PRIME_SPEW("Thread timeout.");

                    if (_numberOfThreadsThatShouldExit > 0) {
                        --_numberOfThreadsThatShouldExit; // Note that _concurrentSemaphore is not increased.
                    } else {
                        _concurrentSemaphore.post(1);
                    }

                    return !_tasks.empty();
                }
            }
        }

        if (task) {
            PRIME_SPEW2("Running task...");
            //CPUUsage().read();
            task();

            haveRunAnyTasks = true;
        }
    }
}

int ThreadPool::enter()
{
    if (ThreadData* data = getThreadData()) {
        return ++data->enterCount;
    }

    return -1;
}

void ThreadPool::leave()
{
    if (ThreadData* data = getThreadData()) {
        --data->enterCount;
    } else {
        PRIME_ASSERTMSG(0, "ThreadPool: leave without enter");
    }
}
}
