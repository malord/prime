// Copyright 2000-2021 Mark H. P. Lord

// Named Clocks.h to not interfere with clock.h in environments which search user header paths for system headers.

#ifndef PRIME_CLOCK_H
#define PRIME_CLOCK_H

#include "Config.h"

#define PRIME_HAVE_CLOCK

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsClock.h"

namespace Prime {
typedef WindowsClock Clock;
}

#elif defined(PRIME_OS_OSX)

#include "OSX/OSXClock.h"

namespace Prime {
typedef OSXClock Clock;
}

#elif defined(PRIME_OS_UNIX)

#include "Unix/UnixClock.h"

namespace Prime {
typedef UnixClock Clock;
}

#else

#undef PRIME_HAVE_CLOCK

#endif

#endif
