// Copyright 2000-2021 Mark H. P. Lord

#include "../Clocks.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

namespace Prime {

#if !defined(PRIME_OS_OSX)

double UnixClock::getMonotonicSeconds() PRIME_NOEXCEPT
{
    return getMonotonicNanosecondTime().toDouble();
}

NanosecondTime UnixClock::getMonotonicNanosecondTime() PRIME_NOEXCEPT
{
    timespec ts;
    PRIME_EXPECT(clock_gettime(CLOCK_MONOTONIC, &ts) == 0);

    return NanosecondTime(ts.tv_sec, ts.tv_nsec);
}

uint32_t UnixClock::getLoopingMonotonicMilliseconds32() PRIME_NOEXCEPT
{
    return static_cast<uint32_t>(getMonotonicMilliseconds64());
}

uint64_t UnixClock::getMonotonicMilliseconds64() PRIME_NOEXCEPT
{
    return getMonotonicNanosecondTime().toMillisecondsU64();
}

#endif

UnixTime UnixClock::getCurrentTime() PRIME_NOEXCEPT
{
    struct timeval tv;
    gettimeofday(&tv, 0);

    return UnixTime(tv.tv_sec, static_cast<NanosecondTime::Nanoseconds>(tv.tv_usec) * 1000);
}

DateTime UnixClock::unixTimeToLocalDateTime(const UnixTime& unixTime)
{
    time_t t = (time_t)unixTime.getSeconds();
    struct tm tm;
    localtime_r(&t, &tm);

    return DateTime(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, unixTime.getFractionNanoseconds());
}

UnixTime UnixClock::localDateTimeToUnixTime(const DateTime& dateTime)
{
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_sec = dateTime.getSecond();
    tm.tm_min = dateTime.getMinute();
    tm.tm_hour = dateTime.getHour();
    tm.tm_mday = dateTime.getDay();
    tm.tm_mon = dateTime.getMonth() - 1;
    tm.tm_year = dateTime.getYear() - 1900;
    tm.tm_isdst = -1;

    return UnixTime(mktime(&tm), dateTime.getNanosecond());
}

void UnixClock::sleepMilliseconds(unsigned int milliseconds) PRIME_NOEXCEPT
{
    usleep((useconds_t)milliseconds * 1000);
}

void UnixClock::sleepMicroseconds(unsigned int microseconds) PRIME_NOEXCEPT
{
    usleep((useconds_t)microseconds);
}
}
