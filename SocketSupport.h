// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SOCKETSUPPORT_H
#define PRIME_SOCKETSUPPORT_H

#include "Config.h"

#define PRIME_HAVE_SOCKET

//#define PRIME_NO_IP6 if IPv6 or the newer APIs (getaddrinfo() etc) are not available.

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsSocketSupport.h"

namespace Prime {
/// Typedef to the SocketSupport type for the platform, which is responsible for initialising socket support.
typedef WindowsSocketSupport SocketSupport;
}

#elif defined(PRIME_OS_UNIX)

#include "Unix/UnixSocketSupport.h"

namespace Prime {
/// Typedef to the SocketSupport type for the platform, which is responsible for initialising socket support.
typedef UnixSocketSupport SocketSupport;
}

#else

#undef PRIME_HAVE_SOCKET

#endif

#endif
