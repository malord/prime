// Copyright 2000-2021 Mark H. P. Lord

#include "UnixSocketSupport.h"

namespace Prime {

bool UnixSocketSupport::initSockets(Log*)
{
    return true;
}

void UnixSocketSupport::shutdownSockets()
{
}

void UnixSocketSupport::logSocketError(Log* log, int err, Log::Level level)
{
    log->logErrno(err, NULL, level);
}

void UnixSocketSupport::logGetAddrInfoError(Log* log, int err, Log::Level level)
{
    log->log(level, "%s", gai_strerror(err));
}

bool UnixSocketSupport::setSocketNonBlocking(Handle handle, bool nonBlocking)
{
    unsigned long parm = nonBlocking ? 1 : 0;
    return ioctlSocket(handle, FIONBIO, (char*)&parm) != -1;
}

UnixSocketSupport::Handle UnixSocketSupport::createSocket(int domain, int type, int protocol, bool forceNoInherit)
{
    UnixCloseOnExec::ScopedLock execLock(forceNoInherit);

#ifdef SOCK_CLOEXEC
    if (forceNoInherit) {
        type |= SOCK_CLOEXEC;
    }
#endif

    Handle sh = ::socket(domain, type, protocol);

    if (forceNoInherit) {
        UnixCloseOnExec::closeOnExec(sh);
    }

    return sh;
}

UnixSocketSupport::Handle UnixSocketSupport::acceptSocket(Handle handle, struct sockaddr* address, AddressLength* addressLength, bool forceNoInherit)
{
#ifdef SOCK_CLOEXEC
    Handle sh = ::accept4(handle, address, addressLength, forceNoInherit ? SOCK_CLOEXEC : 0);
#else
    Handle sh;
    {
        UnixCloseOnExec::ScopedLock execLock(forceNoInherit);

        sh = ::accept(handle, address, addressLength);

        if (sh != -1 && forceNoInherit) {
            UnixCloseOnExec::closeOnExec(sh);
        }
    }
#endif

    return sh;
}
}
