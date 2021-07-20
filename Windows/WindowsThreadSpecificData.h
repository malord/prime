// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSTHREADSPECIFICDATA_H
#define PRIME_WINDOWS_WINDOWSTHREADSPECIFICDATA_H

#include "../Log.h"
#include "WindowsConfig.h"

namespace Prime {

/// Keeps track of thread specific data. Each thread that accesses the data from this object will access its own
/// thread specific value.
class PRIME_PUBLIC WindowsThreadSpecificData {
public:
    /// This function is invoked if the thread is destroyed with a non-null value in the thread's data.
    typedef void (*ThreadDestroyedCallback)(void* data);

    WindowsThreadSpecificData()
        : _tls(TLS_OUT_OF_INDEXES)
    {
    }

    explicit WindowsThreadSpecificData(Log* log, ThreadDestroyedCallback threadDestroyedCallback = NULL,
        const char* debugName = NULL)
        : _tls(TLS_OUT_OF_INDEXES)
    {
        PRIME_EXPECT(init(log, threadDestroyedCallback, debugName));
    }

    ~WindowsThreadSpecificData();

    bool init(Log* log, ThreadDestroyedCallback threadDestroyedCallback = NULL, const char* debugName = NULL);

    bool isInitialised() const { return _tls != TLS_OUT_OF_INDEXES; }

    /// Set the data for the calling thread.
    void set(void* data);

    /// Returns the data of the calling thread.
    void* get();

    /// If the calling thread's data is non-null, invoke the destructor and clear the data.
    void clear();

private:
    ThreadDestroyedCallback _callback;

    DWORD _tls;

    PRIME_UNCOPYABLE(WindowsThreadSpecificData);
};

}

#endif
