// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PTHREADS_PTHREADSTHREADSPECIFICDATA_H
#define PRIME_PTHREADS_PTHREADSTHREADSPECIFICDATA_H

#include "../Log.h"
#include <pthread.h>

namespace Prime {

/// Keeps track of thread specific data. Each thread that accesses the data from this object will access its
/// own thread specific value.
class PRIME_PUBLIC PthreadsThreadSpecificData {
public:
    /// This function is invoked if the thread is destroyed with a non-
    /// null value in the thread's data.
    typedef void (*ThreadDestroyedCallback)(void* data);

    /// Initialise, optionally specifying the callback to be called when a thread is destroyed.
    PthreadsThreadSpecificData()
        : _initialised(false)
    {
    }

    explicit PthreadsThreadSpecificData(Log* log, ThreadDestroyedCallback threadDestroyedCallback = NULL,
        const char* debugName = NULL)
        : _initialised(false)
    {
        PRIME_EXPECT(init(log, threadDestroyedCallback, debugName));
    }

    ~PthreadsThreadSpecificData();

    bool init(Log* log, ThreadDestroyedCallback threadDestroyedCallback = NULL, const char* debugName = NULL);

    bool isInitialised() const { return _initialised; }

    /// Set the data for the calling thread.
    void set(void* data);

    /// Returns the data of the calling thread.
    void* get();

    /// If the calling thread's data is non-null, invoke the destructor
    /// and clear the data.
    void clear();

private:
    ThreadDestroyedCallback _callback;

    pthread_key_t _key;
    bool _initialised;

    PRIME_UNCOPYABLE(PthreadsThreadSpecificData);
};

}

#endif
