// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSSOCKETSUPPORT_H
#define PRIME_WINDOWS_WINDOWSSOCKETSUPPORT_H

#include "../Log.h"
#include "WindowsConfig.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#if WINVER < PRIME_WINVER_FOR(PRIME_WINDOWS_VISTA)
#define PRIME_NO_IP6
#endif

// TODO: don't have getifaddrs on Windows but do have GetAdaptersAddresses. Need a wrapper.
// https://msdn.microsoft.com/en-us/library/aa365915(v=vs.85).aspx

namespace Prime {

class PRIME_PUBLIC WindowsSocketSupport {
public:
    /// Type for socket address lengths.
    typedef int AddressLength;

    /// Platform's socket handle type.
    typedef SOCKET Handle;

    /// Special "bad" value for a socket handle.
    PRIME_STATIC_CONST(Handle, invalidHandle, INVALID_SOCKET);

    /// Type for socket buffer sizes.
    typedef int BufferSize;

    /// The largest possible socket address.
    enum { maxAddressLength = 256 };

    /// Maximum length of a host name.
    enum { maxHostName = 256 };

    /// Maximum connection backlog.
    enum { maxListenBacklog = SOMAXCONN };

    /// Type for error values.
    typedef int ErrorCode;

    enum {
        ErrorInterrupt = WSAEINTR,
        ErrorWouldBlock = WSAEWOULDBLOCK,
        ErrorAddressInUse = WSAEADDRINUSE,
        ErrorConnectionReset = WSAECONNRESET
    };

    /// Returns the error code of the last socket error.
    static ErrorCode getLastSocketError()
    {
        return WSAGetLastError();
    }

    static Handle createSocket(int domain, int type, int protocol, bool = true)
    {
        return ::socket(domain, type, protocol);
    }

    static Handle acceptSocket(Handle handle, struct sockaddr* address, AddressLength* addressLength, bool = true)
    {
        return ::accept(handle, address, addressLength);
    }

    /// Wrapper around ioctlsocket to abstract differences between winsock and BSD sockets.
    static int ioctlSocket(SOCKET handle, unsigned long request, char* argp)
    {
        return ioctlsocket(handle, request, (u_long*)argp);
    }

    /// Wrapper around closesocket to abstract differences between winsock and BSD sockets.
    static int closeSocket(SOCKET handle)
    {
        return closesocket(handle);
    }

    /// Wrapper around select().
    static int selectSocket(int nfds, struct fd_set* readfds, struct fd_set* writefds, struct fd_set* exceptfds,
        struct timeval* timeout)
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

    explicit WindowsSocketSupport(Log* log = Log::getGlobal())
    {
        initSockets(log);
    }

    ~WindowsSocketSupport()
    {
        shutdownSockets();
    }
};
}

#endif
