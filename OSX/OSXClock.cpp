// Copyright 2000-2021 Mark H. P. Lord

#include "OSXClock.h"
#include "../NumberUtils.h"
#include <cmath>
#include <mach/mach_time.h>

namespace Prime {

namespace {
    mach_timebase_info_data_t timebase_info;
}

double OSXClock::getMonotonicSeconds() PRIME_NOEXCEPT
{
    if (timebase_info.denom == 0) {
        mach_timebase_info(&timebase_info);
    }

    // This is the case on all modern Darwins
    if (timebase_info.numer == timebase_info.denom) {
        return mach_absolute_time() / 1e9;
    }

    return mach_absolute_time() * (static_cast<double>(timebase_info.numer) / (static_cast<double>(timebase_info.denom) * 1e9));
}

NanosecondTime OSXClock::getMonotonicNanosecondTime() PRIME_NOEXCEPT
{
    if (timebase_info.denom == 0) {
        mach_timebase_info(&timebase_info);
    }

    // This is the case on all modern Darwins
    if (timebase_info.numer == timebase_info.denom) {
        uint64_t now = mach_absolute_time();

        return NanosecondTime(Quantise<NanosecondTime::Seconds>(now / UINT64_C(1000000000)),
            Quantise<NanosecondTime::Nanoseconds>(now % UINT64_C(1000000000)));
    }

    return NanosecondTime(getMonotonicSeconds());
}

uint32_t OSXClock::getLoopingMonotonicMilliseconds32() PRIME_NOEXCEPT
{
    return static_cast<uint32_t>(getMonotonicMilliseconds64());
}

uint64_t OSXClock::getMonotonicMilliseconds64() PRIME_NOEXCEPT
{
    return getMonotonicNanosecondTime().toMillisecondsU64();
}
}
