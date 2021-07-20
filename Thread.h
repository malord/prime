// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_THREAD_H
#define PRIME_THREAD_H

#include "Config.h"

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsThread.h"

namespace Prime {
/// Typedef to the platform Thread type.
typedef WindowsThread Thread;
}

#elif defined(PRIME_OS_UNIX)

#include "Pthreads/PthreadsThread.h"

namespace Prime {
/// Typedef to the platform Thread type.
typedef PthreadsThread Thread;
}

#endif

#endif
