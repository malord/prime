// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIXTIME_H
#define PRIME_UNIXTIME_H

#include "ByteOrder.h"
#include <math.h>

namespace Prime {

const int secondsPerMinute = 60;
const int secondsPerHour = 60 * secondsPerMinute;
const int secondsPerDay = 24 * secondsPerHour;
const int secondsPerWeek = 7 * secondsPerDay;
const int secondsPerNonLeapYear = 365 * secondsPerDay;
const int secondsPerLeapYear = 365 * secondsPerDay;

const int millisecondsPerSecond = 1000;

//
// NanosecondTime
//

/// A high precision count of nanoseconds with no defined meaning for zero.
class NanosecondTime {
public:
    typedef int64_t Seconds;
    typedef int32_t Nanoseconds;

    static const NanosecondTime distantPast;
    static const NanosecondTime distantFuture;

    static NanosecondTime createMinutes(Seconds n)
    {
        return NanosecondTime(n * secondsPerMinute, 0);
    }

    static NanosecondTime createHours(Seconds n)
    {
        return NanosecondTime(n * secondsPerHour, 0);
    }

    static NanosecondTime createDays(Seconds n)
    {
        return NanosecondTime(n * secondsPerDay, 0);
    }

    static NanosecondTime createMillisecondsU64(uint64_t ms);

    NanosecondTime() PRIME_NOEXCEPT : _seconds(0),
                                      _nanoseconds(0)
    {
    }

    NanosecondTime(const NanosecondTime& copy) PRIME_NOEXCEPT : _seconds(copy._seconds),
                                                                _nanoseconds(copy._nanoseconds)
    {
    }

    explicit NanosecondTime(Seconds s, Nanoseconds n) PRIME_NOEXCEPT : _seconds(s),
                                                                       _nanoseconds(n)
    {
    }

    template <typename Integer>
    explicit NanosecondTime(Integer s, Nanoseconds n) PRIME_NOEXCEPT : _seconds(s),
                                                                       _nanoseconds(n)
    {
    }

    explicit NanosecondTime(double seconds) PRIME_NOEXCEPT;

    explicit NanosecondTime(float seconds) PRIME_NOEXCEPT;

    bool isZero() const PRIME_NOEXCEPT { return _seconds == 0 && _nanoseconds == 0; }

    Seconds getSeconds() const PRIME_NOEXCEPT { return _seconds; }

    Nanoseconds getFractionNanoseconds() const PRIME_NOEXCEPT { return _nanoseconds; }

    Nanoseconds getFractionMicroseconds() const PRIME_NOEXCEPT { return _nanoseconds / 1000; }

    Nanoseconds getFractionMilliseconds() const PRIME_NOEXCEPT { return _nanoseconds / 1000000; }

    double toDouble() const PRIME_NOEXCEPT { return static_cast<double>(_seconds) + static_cast<double>(_nanoseconds) / 1e9; }

    void set(Seconds seconds, Nanoseconds nanoseconds) PRIME_NOEXCEPT
    {
        _seconds = seconds;
        _nanoseconds = nanoseconds;
    }

    void setSeconds(Seconds seconds) PRIME_NOEXCEPT { _seconds = seconds; }

    void setNanoseconds(Nanoseconds nanoseconds) PRIME_NOEXCEPT { _nanoseconds = nanoseconds; }

    bool operator==(const NanosecondTime& rhs) const PRIME_NOEXCEPT
    {
        return _seconds == rhs._seconds && _nanoseconds == rhs._nanoseconds;
    }

    bool operator<(const NanosecondTime& rhs) const PRIME_NOEXCEPT
    {
        if (_seconds < rhs._seconds) {
            return true;
        }
        if (_seconds > rhs._seconds) {
            return false;
        }

        if (_nanoseconds < rhs._nanoseconds) {
            return true;
        }

        return false;
    }

    PRIME_IMPLIED_COMPARISONS_OPERATORS(const NanosecondTime&)

    NanosecondTime operator+(const NanosecondTime& rhs) const PRIME_NOEXCEPT;

    NanosecondTime operator-(const NanosecondTime& rhs) const PRIME_NOEXCEPT;

    NanosecondTime& operator+=(const NanosecondTime& rhs) PRIME_NOEXCEPT;

    NanosecondTime& operator-=(const NanosecondTime& rhs) PRIME_NOEXCEPT;

    uint64_t toMillisecondsU64() const;

protected:
    // For negative times, -1.3 would be represented as -2, 700000000.
    Seconds _seconds;
    Nanoseconds _nanoseconds;
};

//
// UnixTime
//

/// A high precision count of nanoseconds since midnight, 1970-01-01 GMT used to refer to a specific instant in
/// time.
class UnixTime : public NanosecondTime {
public:
    template <typename FileTime> // Template to avoid needing to include Windows headers.
    static UnixTime fromWindowsFileTime(const FileTime& ft) PRIME_NOEXCEPT
    {
        return fromWindowsFileTime64(PRIME_MAKE64(ft.dwLowDateTime, ft.dwHighDateTime));
    }

    static UnixTime fromWindowsFileTime64(uint64_t whole) PRIME_NOEXCEPT;

    UnixTime() PRIME_NOEXCEPT
    {
    }

    UnixTime(const UnixTime& copy) PRIME_NOEXCEPT : NanosecondTime(copy)
    {
    }

    explicit UnixTime(const NanosecondTime& copy) PRIME_NOEXCEPT : NanosecondTime(copy)
    {
    }

    explicit UnixTime(Seconds s, Nanoseconds n = 0) PRIME_NOEXCEPT : NanosecondTime(s, n)
    {
    }

    template <typename Integer>
    explicit UnixTime(Integer s, Nanoseconds n = 0) PRIME_NOEXCEPT : NanosecondTime(s, n)
    {
    }

    explicit UnixTime(double seconds) PRIME_NOEXCEPT : NanosecondTime(seconds)
    {
    }

    explicit UnixTime(float seconds) PRIME_NOEXCEPT : NanosecondTime(seconds)
    {
    }

    template <typename FileTime> // Template to avoid needing to include Windows headers.
    void toWindowsFileTime(FileTime& ft) const PRIME_NOEXCEPT
    {
        uint64_t filetime64 = toWindowsFileTime64();
        ft.dwLowDateTime = (uint32_t)(filetime64);
        ft.dwHighDateTime = (uint32_t)(filetime64 >> 32);
    }

    uint64_t toWindowsFileTime64() const PRIME_NOEXCEPT;

    UnixTime getMidnight() const PRIME_NOEXCEPT
    {
        return UnixTime(getSeconds() / secondsPerDay * secondsPerDay, getFractionNanoseconds());
    }

    UnixTime getMidday() const PRIME_NOEXCEPT
    {
        return getMidnight() + NanosecondTime::createHours(12);
    }

    UnixTime operator+(const NanosecondTime& rhs) const PRIME_NOEXCEPT;

    UnixTime operator-(const NanosecondTime& rhs) const PRIME_NOEXCEPT;

    UnixTime& operator+=(const NanosecondTime& rhs) PRIME_NOEXCEPT;

    UnixTime& operator-=(const NanosecondTime& rhs) PRIME_NOEXCEPT;
};
}

#endif
