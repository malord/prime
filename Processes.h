// Copyright 2000-2021 Mark H. P. Lord

// (Can't call this file Process.h due to platforms with faulty #include implementations.)

#ifndef PRIME_PROCESSES_H
#define PRIME_PROCESSES_H

#include "Config.h"

#define PRIME_HAVE_PROCESS

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsProcess.h"

namespace Prime {
/// Typedef to the platform Process implementation.
typedef WindowsProcess Process;
}

#elif defined(PRIME_OS_UNIX)

#include "Unix/UnixProcess.h"

namespace Prime {
/// Typedef to the platform Process implementation.
typedef UnixProcess Process;
}

#else

#undef PRIME_HAVE_PROCESS

#endif

#endif
