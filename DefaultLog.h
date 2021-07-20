// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DefaultLOG_H
#define PRIME_DefaultLOG_H

#include "Config.h"

#if defined(PRIME_OS_XBOX)

#include "Windows/WindowsLog.h"

namespace Prime {
typedef WindowsLog DefaultLog;
}

#elif defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsLog.h"

namespace Prime {
// In windowed applications, this will figure out that stdout/stderr are unavailable and will write to
// OutputDebugString instead.
typedef WindowsLog DefaultLog;
}

#elif defined(PRIME_OS_UNIX)

#include "Unix/UnixLog.h"

namespace Prime {
typedef UnixLog DefaultLog;
}

#else

#include "StdioLog.h"

namespace Prime {
typedef StdioLog DefaultLog;
}

#endif

#endif
