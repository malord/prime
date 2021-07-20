// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXCLOCK_H
#define PRIME_UNIX_UNIXCLOCK_H

#include "../DateTime.h"

namespace Prime {

class PRIME_PUBLIC UnixClock {
public:
// OS X implements these separately
#if !defined(PRIME_OS_OSX)

    /// Returns the current value of a high resolution clock in seconds. The meaning of zero is not defined.
    static double getMonotonicSeconds() PRIME_NOEXCEPT;

    /// Returns a monotonic time as a NanosecondTime. The resolution of this time is disappointing on some platforms.
    static NanosecondTime getMonotonicNanosecondTime() PRIME_NOEXCEPT;

    /// Returns the current value of a millisecond clock. The clock will loop at 0xffffffff.
    static uint32_t getLoopingMonotonicMilliseconds32() PRIME_NOEXCEPT;

    /// Returns the current value of a millisecond clock. Will not loop for billions of years.
    static uint64_t getMonotonicMilliseconds64() PRIME_NOEXCEPT;

#endif

    /// Return the current system time as a UnixTime (number of seconds since 1/1/1970, in UTC). The resolution of
    /// this time is disappointing on some platforms.
    static UnixTime getCurrentTime() PRIME_NOEXCEPT;

    /// Convert a UnixTime (UTC) to a DateTime in local time (e.g., UNIX's localtime()).
    static DateTime unixTimeToLocalDateTime(const UnixTime& unixTime);

    /// Convert a DateTime in local time to a UnixTime (UTC) (e.g., UNIX's mktime()).
    static UnixTime localDateTimeToUnixTime(const DateTime& dateTime);

    /// Sleep for the specified number of 1,000th of a second.
    static void sleepMilliseconds(unsigned int milliseconds) PRIME_NOEXCEPT;

    /// Sleep for the specified number of 1,000,000th of a second.
    static void sleepMicroseconds(unsigned int microseconds) PRIME_NOEXCEPT;
};
}

#endif
