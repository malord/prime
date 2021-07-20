// Copyright 2000-2021 Mark H. P. Lord

#include "../Clocks.h"
#include "WindowsCriticalSection.h"
#include <time.h> // TODO: use Win32 API

#if !PRIME_MSC_AND_OLDER(1300)
#define USE_PERFORMANCE_COUNTER
#endif

// You'll need this for < Windows XP
#define USE_MULTIMEDIA_TIMER

// "nonstandard extension used: nameless struct/union"
#ifdef _MSC_VER
#pragma warning(disable : 4201)
#endif

#include <mmsystem.h>

#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

namespace Prime {

namespace {

#ifdef USE_MULTIMEDIA_TIMER

    //
    // MultimediaTimerClock
    //

    class MultimediaTimerClock {
    public:
        static MultimediaTimerClock* getSingleton();

        uint64_t getLoopingMilliseconds64()
        {
            WindowsCriticalSection::ScopedLock lock(&_mutex);

            uint32_t currentTime = timeGetTime();
            _milliseconds64 += currentTime - _lastTime;
            _lastTime = currentTime;

            return _milliseconds64;
        }

    private:
        MultimediaTimerClock()
        {
            timeBeginPeriod(1);
            _lastTime = timeGetTime();
            _milliseconds64 = 0;
        }

        ~MultimediaTimerClock()
        {
            timeEndPeriod(1);
        }

        WindowsCriticalSection _mutex;
        uint32_t _lastTime;
        uint64_t _milliseconds64;
    };

    MultimediaTimerClock* MultimediaTimerClock::getSingleton()
    {
        static MultimediaTimerClock wcm;
        return &wcm;
    }

#endif

#ifdef USE_PERFORMANCE_COUNTER

    //
    // PerformanceCounterClock
    //

    class PerformanceCounterClock {
    public:
        static PerformanceCounterClock* getSingleton();

        /// Prior to Windows XP, performance counters weren't guaranteed to be available.
        bool isInitialised() const { return _initialised; }

        double getSeconds() const;

        NanosecondTime getNanosecondTime() const;

    private:
        PerformanceCounterClock();

        bool _initialised;
        double _frequency;
    };

    PerformanceCounterClock* PerformanceCounterClock::getSingleton()
    {
        static PerformanceCounterClock pcc;
        return pcc.isInitialised() ? &pcc : NULL;
    }

    PerformanceCounterClock::PerformanceCounterClock()
    {
        LARGE_INTEGER frequency;
        _initialised = QueryPerformanceFrequency(&frequency) != FALSE;
        if (_initialised) {
            _frequency = static_cast<double>(frequency.QuadPart);
        }
    }

    double PerformanceCounterClock::getSeconds() const
    {
        if (!PRIME_GUARD(isInitialised())) {
            return 0;
        }

        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);

        return static_cast<double>(counter.QuadPart) / _frequency;
    }

    NanosecondTime PerformanceCounterClock::getNanosecondTime() const
    {
        return NanosecondTime(getSeconds());
    }

#endif
}

double WindowsClock::getMonotonicSeconds() PRIME_NOEXCEPT
{
#ifdef USE_PERFORMANCE_COUNTER

    if (PerformanceCounterClock* pcc = PerformanceCounterClock::getSingleton()) {
        return pcc->getSeconds();
    }

#endif

    return getMonotonicNanosecondTime().toDouble();
}

NanosecondTime WindowsClock::getMonotonicNanosecondTime() PRIME_NOEXCEPT
{
#ifdef USE_PERFORMANCE_COUNTER

    if (PerformanceCounterClock* pcc = PerformanceCounterClock::getSingleton()) {
        return pcc->getNanosecondTime();
    }

#endif

#ifdef USE_MULTIMEDIA_TIMER

    return NanosecondTime::createMillisecondsU64(MultimediaTimerClock::getSingleton()->getLoopingMilliseconds64());

#else

    return getCurrentTime();

#endif
}

uint32_t WindowsClock::getLoopingMonotonicMilliseconds32() PRIME_NOEXCEPT
{
    // Rather than just calling timeGetTime(), call getMonotonicMilliseconds64() to make sure the 64-bit clock
    // gets updated as often as possible.
    return static_cast<uint32_t>(getMonotonicMilliseconds64());
}

uint64_t WindowsClock::getMonotonicMilliseconds64() PRIME_NOEXCEPT
{
    return getMonotonicNanosecondTime().toMillisecondsU64();
}

UnixTime WindowsClock::getCurrentTime() PRIME_NOEXCEPT
{
    FILETIME ft;
    memset(&ft, 0, sizeof(ft));
    GetSystemTimeAsFileTime(&ft);
    return UnixTime::fromWindowsFileTime(ft);
}

void WindowsClock::sleepMilliseconds(unsigned int milliseconds) PRIME_NOEXCEPT
{
    Sleep(milliseconds);
}

// void WindowsClock::sleepMicroseconds(unsigned int microseconds) PRIME_NOEXCEPT
// {
//  Sleep((microseconds + 500) / 1000); // Lame!
// }

DateTime WindowsClock::unixTimeToLocalDateTime(const UnixTime& unixTime)
{
    // As of VC 2005, time_t is 64-bit, so this just works.
    time_t t = (time_t)unixTime.getSeconds();
    struct tm tm = *localtime(&t);

    return DateTime(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, unixTime.getNanoseconds());
}

UnixTime WindowsClock::localDateTimeToUnixTime(const DateTime& dateTime)
{
    // As of VC 2005, time_t is 64-bit, so this just works.
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
}
