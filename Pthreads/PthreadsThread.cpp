// Copyright 2000-2021 Mark H. P. Lord

#include "PthreadsThread.h"
#include "../ScopedPtr.h"
#include "../StringUtils.h"
#include <signal.h>
#if defined(PRIME_OS_MAC) || defined(PRIME_OS_IOS)
#include <errno.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#endif
#ifdef PRIME_OS_LINUX
#include <errno.h>
#include <unistd.h>
#endif

#ifndef NDBEUG
#define ENABLE_THREAD_NAMES
#endif

namespace Prime {

#if defined(PRIME_OS_MAC) || defined(PRIME_OS_IOS)

int PthreadsThread::getCPUCount(Log* log)
{
    int count = -1;
    size_t size = sizeof(count);

    if (sysctlbyname("hw.ncpu", &count, &size, 0, 0) != 0) {
        log->logErrno(errno, "sysctlbyname");
        return -1;
    }

    return count;
}

#elif defined(PRIME_OS_LINUX)

int PthreadsThread::getCPUCount(Log* log)
{
    long int result = sysconf(_SC_NPROCESSORS_ONLN);
    if (result <= 0 || (int)result != result) {
        log->logErrno(errno, "sysconf");
        return -1;
    }

    return Narrow<int>(result);
}

#elif defined(PRIME_OS_PS3_PPU)

int PthreadsThread::getCPUCount(Log*)
{
    return 2;
}

#elif defined(PRIME_OS_PS3_SPU)

int PthreadsThread::getCPUCount(Log*)
{
    return 1;
}

#else

int PthreadsThread::getCPUCount(Log* log)
{
    log->error("CPU count not available on this platform.");
    return -1;
}

#endif

PthreadsThread::PthreadsThread()
    : _attached(false)
{
}

PthreadsThread::~PthreadsThread()
{
    if (_attached) {
        PRIME_EXPECT(pthread_detach(_thread) == 0);
    }
}

bool PthreadsThread::isRunning()
{
    if (!_attached) {
        return false;
    }

    return pthread_kill(_thread, 0) == 0;
}

bool PthreadsThread::join()
{
    if (!_attached) {
        return true;
    }

    if (pthread_join(_thread, 0) != 0) {
        return false;
    }

    _attached = false;
    return true;
}

bool PthreadsThread::cancel()
{
    if (!_attached) {
        return true;
    }

#ifdef PRIME_OS_ANDROID
    Log::getGlobal()->developerWarning("pthread_cancel not available");
    return false;
#else
    return pthread_cancel(_thread) == 0;
#endif
}

namespace {

    using namespace Prime;

    struct ThunkData {
        PthreadsThread::Callback callback;
#ifdef ENABLE_THREAD_NAMES
        ScopedArrayPtr<char> debugName;
#endif
    };
}

bool PthreadsThread::create(void (*entryPoint)(void*), void* context, size_t stackSize, Log* log, const char* debugName)
{
#ifdef PRIME_CXX11_STL
    return create([entryPoint, context]() { (*entryPoint)(context); }, stackSize, log, debugName);
#else
    return create(FunctionCallback(entryPoint, context), stackSize, log, debugName);
#endif
}

bool PthreadsThread::create(const Callback& callback, size_t stackSize, Log* log, const char* debugName)
{
    PRIME_ASSERT(!_attached);

    ScopedPtr<ThunkData> td(new ThunkData);

    td->callback = callback;
#ifdef ENABLE_THREAD_NAMES
    td->debugName.reset(debugName ? NewString(debugName) : NULL);
#endif

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    if (stackSize != 0) {
        pthread_attr_setstacksize(&attr, stackSize);
    }

    _attached = true;
    int result = pthread_create(&_thread, &attr, thunk, td);

    pthread_attr_destroy(&attr);

    if (result != 0) {
        _attached = false;
        log->logErrno(result, debugName);
        return false;
    }

    td.detach();

    return true;
}

void* PthreadsThread::thunk(void* data)
{
    ScopedPtr<ThunkData> td((ThunkData*)data);

#if 0
#ifdef PRIME_OS_BSD
                size_t stackSize = pthread_get_stacksize_np(pthread_self());
                Trace("Stack size: %" PRIuPTR, stackSize);
#endif
#endif

#ifdef ENABLE_THREAD_NAMES
#ifdef PRIME_OS_OSX
    if (td->debugName.get()) {
        pthread_setname_np(td->debugName.get());
    }
#else
    // TODO: more platforms
#endif
    td->debugName.reset();
#endif

    td->callback();

    return 0;
}
}
