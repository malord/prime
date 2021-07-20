// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_THREADSPECIFICDATA_H
#define PRIME_THREADSPECIFICDATA_H

#include "Config.h"

namespace Prime {

class Log;

/// Emulates the platform specific ThreadSpecificData classes.
class NullThreadSpecificData {
public:
    /// This function is invoked if the thread is destroyed with a non-null value in the thread's data.
    typedef void (*ThreadDestroyedCallback)(void* data);

    /// Initialise, optionally specifying the callback to be called when a thread is destroyed.
    NullThreadSpecificData()
        : _callback(0)
        , _data(0)
    {
    }

    ~NullThreadSpecificData() { clear(); }

    bool init(Log* log, ThreadDestroyedCallback threadDestroyedCallback = NULL, const char* debugName = NULL)
    {
        (void)log;
        (void)debugName;
        clear();
        _callback = threadDestroyedCallback;
        return true;
    }

    bool isCreated() const { return true; }

    /// Set the data for the calling thread.
    void set(void* data)
    {
        clear();
        _data = data;
    }

    /// Returns the data of the calling thread.
    void* get() { return _data; }

    /// If the calling thread's data is non-null, invoke the destructor and clear the data.
    void clear()
    {
        if (_data && _callback) {
            _callback(_data);
        }

        _data = 0;
    }

private:
    ThreadDestroyedCallback _callback;
    void* _data;

    PRIME_UNCOPYABLE(NullThreadSpecificData);
};
}

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsThreadSpecificData.h"

namespace Prime {
/// Typedef to the platform ThreadSpecificData.
typedef WindowsThreadSpecificData ThreadSpecificData;
}

#elif defined(PRIME_OS_UNIX)

#include "Pthreads/PthreadsThreadSpecificData.h"

namespace Prime {
/// Typedef to the platform ThreadSpecificData.
typedef PthreadsThreadSpecificData ThreadSpecificData;
}

#else

// TODO: can emulate using Thread::ThreadIDs as a map key

namespace Prime {
/// Typedef to the platform ThreadSpecificData.
typedef NullThreadSpecificData ThreadSpecificData;
}

#endif

#endif
