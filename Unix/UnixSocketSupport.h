// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXSOCKETSUPPORT_H
#define PRIME_UNIX_UNIXSOCKETSUPPORT_H

#include "../Log.h"
#include "UnixCloseOnExec.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(PRIME_OS_OSX) || defined(PRIME_OS_BSD)
#define PRIME_OS_HAS_GETIFADDRS
#include <ifaddrs.h>
#include <sys/uio.h>
#endif

#if defined(PRIME_OS_LINUX)
#define PRIME_OS_HAS_GETIFADDRS
#include <ifaddrs.h>
#include <sys/sendfile.h>
#endif

namespace Prime {

class PRIME_PUBLIC UnixSocketSupport {
public:
    /// Type for socket address lengths.
    typedef socklen_t AddressLength;

    /// Platform's socket handle type.
    typedef int Handle;

    /// Special "bad" value for a socket handle.
    PRIME_STATIC_CONST(Handle, invalidHandle, -1);

    /// Type for socket buffer sizes.
    typedef size_t BufferSize;

    /// The largest possible socket address.
    PRIME_STATIC_CONST(int, maxHostName, MAXHOSTNAMELEN);

    /// Maximum length of a host name.
    PRIME_STATIC_CONST(AddressLength, maxAddressLength, sizeof(sockaddr_storage));

    /// Maximum connection backlog.
    PRIME_STATIC_CONST(int, maxListenBacklog, SOMAXCONN);

    /// Type for error values.
    typedef int ErrorCode;

    enum {
        ErrorInterrupt = EINTR,
        ErrorWouldBlock = EWOULDBLOCK,
        ErrorAddressInUse = EADDRINUSE,
        ErrorConnectionReset = ECONNRESET
    };

    /// Returns the error code of the last socket error.
    static ErrorCode getLastSocketError()
    {
        return errno;
    }

    static Handle createSocket(int domain, int type, int protocol, bool forceNoInherit);

    static Handle acceptSocket(Handle handle, struct sockaddr* address, AddressLength* addressLength, bool forceNoInherit);

    /// Wrapper around ioctl to abstract differences between winsock and BSD sockets.
    static int ioctlSocket(Handle handle, unsigned long request, char* argp)
    {
        return ioctl(handle, request, argp);
    }

    /// Wrapper around close() to abstract differences between winsock and BSD sockets.
    static int closeSocket(Handle handle)
    {
        return close(handle);
    }

    /// Wrapper around select().
    static int selectSocket(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout)
    {
        return ::select(nfds, readfds, writefds, exceptfds, timeout);
    }

    /// Set a socket's non-blocking mode.
    static bool setSocketNonBlocking(Handle handle, bool nonBlocking);

    /// Log a description of a socket error.
    static void logSocketError(Log* log, int err, Log::Level level = Log::LevelError);

    /// Log an error returned by getaddrinfo.
    static void logGetAddrInfoError(Log* log, int err, Log::Level level = Log::LevelError);

    static bool initSockets(Log* log);

    static void shutdownSockets();

    explicit UnixSocketSupport(Log* log = Log::getGlobal())
    {
        initSockets(log);
    }

    ~UnixSocketSupport()
    {
        shutdownSockets();
    }
};
}

#endif
