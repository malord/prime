// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OSX_OSXCLOCK_H
#define PRIME_OSX_OSXCLOCK_H

#include "../Unix/UnixClock.h"

namespace Prime {

class PRIME_PUBLIC OSXClock : public UnixClock {
public:
    /// Returns the current value of a high resolution clock in seconds. The meaning of zero is not defined.
    static double getMonotonicSeconds() PRIME_NOEXCEPT;

    /// Returns a monotonic time as a NanosecondTime. The resolution of this time is disappointing on some platforms.
    static NanosecondTime getMonotonicNanosecondTime() PRIME_NOEXCEPT;

    /// Returns the current value of a millisecond clock. The clock will loop at 0xffffffff.
    static uint32_t getLoopingMonotonicMilliseconds32() PRIME_NOEXCEPT;

    /// Returns the current value of a millisecond clock. Will not loop for billions of years.
    static uint64_t getMonotonicMilliseconds64() PRIME_NOEXCEPT;
};
}

#endif
