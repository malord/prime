// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PTHREADS_PTHREADSTIME_H
#define PRIME_PTHREADS_PTHREADSTIME_H

#include "../Config.h"
#if defined(PRIME_OS_PS3)
#include <sys/sys_time.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

namespace Prime {

/// Compute the time milliseconds in to the future and store it in *ts.
PRIME_PUBLIC bool ComputeTimespecAfterMilliseconds(int milliseconds, timespec& ts);

}

#endif
