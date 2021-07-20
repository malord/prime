// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TASKQUEUE_H
#define PRIME_TASKQUEUE_H

#include "Lock.h"
#include "Semaphore.h"
#ifndef PRIME_CXX11_STL
#include "Callback.h"
#endif
#include <functional>

namespace Prime {

/// A queue of callbacks to be run. Depending on the kind of queue, the callbacks may run one at a time in strict
/// FIFO order, or multiple callbacks may be run simultaneously in arbitrary order. They may run on a background
/// thread or a specific thread.
class PRIME_PUBLIC TaskQueue : public RefCounted {
public:
#ifdef PRIME_CXX11_STL
    typedef std::function<void()> Callback;
    typedef std::function<void(size_t)> ApplyCallback;
#else
    typedef Callback0<void> Callback;
    typedef Callback1<void, size_t> ApplyCallback;
#endif

    /// Allows you to wait for a group of callbacks to finish. The callbacks can be queued on different queues.
    class PRIME_PUBLIC TaskGroup : public RefCounted {
    public:
        TaskGroup();

        virtual ~TaskGroup();

        /// Queue a callback on a queue and add it to this group. You can queue as many callbacks as you like,
        /// on as many queues you like, and then wait for them all to finish by calling wait().
        virtual void queue(TaskQueue* queue, const Callback& callback) = 0;

        /// Wait for all the queued callbacks to finish (if they haven't already).
        virtual void wait() = 0;

        PRIME_UNCOPYABLE(TaskGroup);
    };

    TaskQueue();

    virtual ~TaskQueue();

    /// Queue a callback to run.
    /// If you need to pass data to the callback or poll for the task to finish then either call a method on your
    /// own object from within the callback or consider subclassing the Task class. Note that a Callback can be
    /// made on a smart pointer, e.g., MethodCallback(Ref(object), &Type::method);
    virtual void queue(const Callback& callback) = 0;

    /// Queue a callback and wait for it to finish. This is useful when you need the callback to run serially
    /// with other callbacks, e.g., file loading from slow media. This is preferable to calling
    /// queueWaitable(callback).wait() since it can deal with situations where that could deadlock, e.g., calling
    /// from on a serial queue from a serial queue.
    virtual void queueAndWait(const Callback& callback) = 0;

    /// Invokes a callback count times with the call number passed as an argument. Returns once all the callbacks
    /// have finished.
    virtual void apply(const ApplyCallback& callback, size_t count) = 0;

    /// Invokes a callback count times with the call number passed as an argument. Returns immediately.
    virtual void queueApply(const ApplyCallback& callback, size_t count,
        const Callback& finishCallback)
        = 0;

    /// Create a TaskGroup, which can be used to wait for any number of callbacks to complete on this, or a
    /// compatible (usually meaning, "within the same TaskSystem"), TaskQueue.
    virtual RefPtr<TaskGroup> createTaskGroup() = 0;

    /// Returned by queueWaitable().
    class PRIME_PUBLIC Waitable {
    public:
        Waitable(const Waitable& copy)
            : _data(copy._data)
        {
        }

        Waitable& operator=(const Waitable& copy)
        {
            _data = copy._data;
            return *this;
        }

#ifdef PRIME_COMPILER_RVALUEREF

        Waitable(Waitable&& move) PRIME_NOEXCEPT : _data(std::move(move._data))
        {
        }

        Waitable& operator=(Waitable&& move) PRIME_NOEXCEPT
        {
            _data = std::move(move._data);
            return *this;
        }

#endif

        void wait()
        {
            _data->wait();
        }

        bool tryWait() { return _data->tryWait(); }

        bool tryWait(int milliseconds) { return _data->tryWait(milliseconds); }

    private:
        friend class TaskQueue;

        Waitable(TaskQueue* queue, const Callback& callback);

        class Data : public RefCounted {
        public:
            explicit Data(const Callback& callback);

            void wait();

            bool tryWait();

            bool tryWait(int milliseconds);

            void run();

        private:
            Callback _callback;
            TimedSemaphore _sem;
        };

        RefPtr<Data> _data;
    };

    /// Queue a task and return a Waitable that can be used to await the result when it's needed.
    /// In the future this might vary per implementation, but at the moment it does not.
    Waitable queueWaitable(const Callback& callback);

    /// Tell the task system that a job running on this queue wishes to yielded. If this method returns true,
    /// you must call resumeDoNotCallDirectly().
    virtual bool yieldDoNotCallDirectly();

    /// Tell that task system that a job that made a successful call to yieldDoNotCallDirectly() now wishes to
    /// resume.
    virtual void resumeDoNotCallDirectly();

    /// A safer way to yield. Use RAII to ensure we resume if we yielded.
    class ScopedYield {
    public:
        explicit ScopedYield(TaskQueue* queue)
        {
            _queue = (queue && queue->yieldDoNotCallDirectly()) ? queue : NULL;
        }

        ~ScopedYield()
        {
            if (_queue) {
                _queue->resumeDoNotCallDirectly();
            }
        }

        void yield(TaskQueue* queue)
        {
            resumeDoNotCallDirectly();
            _queue = queue->yieldDoNotCallDirectly() ? queue : NULL;
        }

        void resumeDoNotCallDirectly()
        {
            if (_queue) {
                _queue->resumeDoNotCallDirectly();
                _queue = NULL;
            }
        }

    private:
        TaskQueue* _queue;

        PRIME_UNCOPYABLE(ScopedYield);
    };

    PRIME_UNCOPYABLE(TaskQueue);
};
}

#endif
