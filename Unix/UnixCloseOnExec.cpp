// Copyright 2000-2021 Mark H. P. Lord

#include "UnixCloseOnExec.h"
#include "../Log.h"
#include "../Mutex.h"
#include "../Templates.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Prime {

static RecursiveMutex* GetCloseOnExecMutex()
{
    // Thread safe in C++11
    static RecursiveMutex mutex(Log::getGlobal());
    return &mutex;
}

void UnixCloseOnExec::globalShutdown()
{
}

void UnixCloseOnExec::globalInit()
{
    GetCloseOnExecMutex();
}

void UnixCloseOnExec::closeOnExec(int fd)
{
#ifdef F_SETFD
    int result = fcntl(fd, F_SETFD, fcntl(fd, F_GETFD, 0) | FD_CLOEXEC);
    if (result == -1) {
        Log::getGlobal()->error(PRIME_LOCALISE("fcntl() failed"));
    }
#endif
}

void UnixCloseOnExec::lock()
{
    GetCloseOnExecMutex()->lock();
}

void UnixCloseOnExec::unlock()
{
    GetCloseOnExecMutex()->unlock();
}
}
