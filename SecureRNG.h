// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SECURERNG_H
#define PRIME_SECURERNG_H

#include "Config.h"

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsSecureRNG.h"

#ifdef PRIME_HAVE_WINDOWSSECURERNG

namespace Prime {
/// Typedef to a secure random number generator for the platform.
typedef WindowsSecureRNG SecureRNG;
}

#else

#define PRIME_NO_SECURERNG

#endif // PRIME_HAVE_WINDOWSSECURERNG

#elif defined(PRIME_OS_UNIX)

#include "Unix/UnixSecureRNG.h"

namespace Prime {
/// Typedef to a secure random number generator for the platform.
typedef UnixSecureRNG SecureRNG;
}

#else

#define PRIME_NO_SECURERNG

#endif

#endif
