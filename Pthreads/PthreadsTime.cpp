// Copyright 2000-2021 Mark H. P. Lord

#include "PthreadsTime.h"

#if defined(PRIME_OS_PS3)

namespace Prime {

bool ComputeTimespecAfterMilliseconds(int milliseconds, timespec* ts)
{
    sys_time_sec_t sec;
    sys_time_nsec_t nsec;

    if (sys_time_get_current_time(&sec, &nsec) != 0) {
        ts->tv_sec = ts->tv_nsec = 0;
        return false;
    }

    if (milliseconds < 0) {
        milliseconds = INT_MAX;
    }

    sec += milliseconds / 1000;
    nsec += (milliseconds % 1000) * 1000000;
    if (nsec > 1000000000) {
        sec++;
        nsec -= 1000000000;
    }

    ts->tv_sec = sec;
    ts->tv_nsec = nsec;
    return true;
}
}

#elif defined(PRIME_OS_UNIX)

namespace Prime {

bool ComputeTimespecAfterMilliseconds(int milliseconds, timespec& ts)
{
    struct timeval tv;

    if (milliseconds < 0) {
        milliseconds = INT_MAX;
    }

    gettimeofday(&tv, 0);
    tv.tv_sec += milliseconds / 1000;
    tv.tv_usec += ((uint32_t)milliseconds % 1000) * 1000;
    tv.tv_sec += tv.tv_usec / 1000000;
    tv.tv_usec %= 1000000;

    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;

    return true;
}

}

#endif
