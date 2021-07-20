// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsThread.h"
#include <memory>
#include <process.h>

#ifndef NDEBUG
#define ENABLE_THREAD_NAMES
#endif

#include "../ScopedPtr.h"

#ifdef ENABLE_THREAD_NAMES

// http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx

namespace {

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(DWORD dwThreadID, const char* threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try {
#if WINVER < 0x500
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(unsigned long), (unsigned long*)&info);
#else
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
#endif
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

}
#endif

namespace Prime {

int WindowsThread::getCPUCount(Log*)
{
#if defined(PRIME_OS_360)
    return 6;
#else
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
#endif
}

WindowsThread::WindowsThread()
    : _handle(0)
{
}

WindowsThread::~WindowsThread()
{
    if (_handle) {
        CloseHandle(_handle);
    }
}

bool WindowsThread::isRunning()
{
    if (!_handle) {
        return false;
    }

    return WaitForSingleObject(_handle, 0) != WAIT_OBJECT_0;
}

bool WindowsThread::join()
{
    if (!_handle) {
        return true;
    }

    return WaitForSingleObject(_handle, INFINITE) == WAIT_OBJECT_0;
}

bool WindowsThread::cancel()
{
    if (!_handle) {
        return true;
    }

#ifdef PRIME_OS_XBOX
    PRIME_ASSERTMSG(0, "TerminateThread not available on Xbox.");
    return false;
#else
    return TerminateThread(_handle, 0) ? true : false;
#endif
}

namespace {

    using namespace Prime;

    struct ThunkData {
        WindowsThread::Callback callback;
    };

    unsigned int __stdcall Thunk(void* data)
    {
        ThunkData* tdData = (ThunkData*)data;
        ThunkData td = *tdData;
        delete tdData;

        td.callback();

        return 0;
    }
}

bool WindowsThread::create(void (*entryPoint)(void*), void* context, size_t stackSize, Log* log, const char* debugName)
{
#ifdef PRIME_CXX11_STL
    return create([entryPoint, context]() { (*entryPoint)(context); }, stackSize, log, debugName);
#else
    return create(FunctionCallback(entryPoint, context), stackSize, log, debugName);
#endif
}

bool WindowsThread::create(const Callback& callback, size_t stackSize, Log* log, const char* debugName)
{
    PRIME_ASSERT(!_handle);

    ScopedPtr<ThunkData> td(new ThunkData);

    td->callback = callback;

    SetLastError(0);

    _handle = (HANDLE)_beginthreadex(0, (unsigned int)stackSize, Thunk, td.get(), 0, &_threadID);

    if (!_handle) {
        log->logErrno(errno, debugName);
        return false;
    }

    td.detach();

#ifdef ENABLE_THREAD_NAMES
    SetThreadName(_threadID, debugName);
#else
    (void)debugName;
#endif

    return true;
}

}
