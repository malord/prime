// Copyright 2000-2021 Mark H. P. Lord

#include "UnixTime.h"

namespace Prime {

static const int32_t oneE9 = 1000000000;
static const int32_t oneE7 = 10000000;
static const int32_t oneE6 = 1000000;

//
// NanosecondTime
//

const NanosecondTime NanosecondTime::distantPast(INT64_MIN, 0);
const NanosecondTime NanosecondTime::distantFuture(INT64_MAX, 0);

NanosecondTime NanosecondTime::createMillisecondsU64(uint64_t ms)
{
    return NanosecondTime(Narrow<Seconds>(ms / 1000),
        Narrow<Nanoseconds>(ms % 1000) * oneE6);
}

NanosecondTime::NanosecondTime(double seconds) PRIME_NOEXCEPT
{
    _seconds = (Seconds)floor(seconds);
    _nanoseconds = (int32_t)((seconds - _seconds) * 1e9);
}

NanosecondTime::NanosecondTime(float seconds) PRIME_NOEXCEPT
{
    _seconds = (Seconds)floorf(seconds);
    _nanoseconds = (int32_t)((seconds - _seconds) * 1e9);
}

NanosecondTime NanosecondTime::operator-(const NanosecondTime& rhs) const PRIME_NOEXCEPT
{
    NanosecondTime out(_seconds - rhs._seconds, _nanoseconds - rhs._nanoseconds);
    if (out._nanoseconds < 0) {
        out._seconds -= 1;
        out._nanoseconds += oneE9;
    }
    return out;
}

NanosecondTime NanosecondTime::operator+(const NanosecondTime& rhs) const PRIME_NOEXCEPT
{
    NanosecondTime out;
    out._seconds = _seconds + rhs._seconds;
    out._nanoseconds = _nanoseconds + rhs._nanoseconds;
    if (out._nanoseconds > oneE9) {
        out._seconds += 1;
        out._nanoseconds -= oneE9;
    }
    return out;
}

NanosecondTime& NanosecondTime::operator+=(const NanosecondTime& rhs) PRIME_NOEXCEPT
{
    *this = operator+(rhs);
    return *this;
}

NanosecondTime& NanosecondTime::operator-=(const NanosecondTime& rhs) PRIME_NOEXCEPT
{
    *this = operator-(rhs);
    return *this;
}

uint64_t NanosecondTime::toMillisecondsU64() const
{
    if (!PRIME_GUARD(_seconds > 0 && _nanoseconds > 0)) {
        return 0;
    }

    return static_cast<uint64_t>(_nanoseconds / oneE6) + static_cast<uint64_t>(_seconds) * UINT64_C(1000);
}

//
// UnixTime
//

UnixTime UnixTime::fromWindowsFileTime64(uint64_t whole) PRIME_NOEXCEPT
{
    int64_t hundredNanos = (int64_t)whole;
    hundredNanos -= INT64_C(116444736000000000);

    Seconds seconds = (Seconds)(hundredNanos / oneE7);
    int32_t nanoseconds = (int32_t)((hundredNanos % oneE7) * 100);

    return UnixTime(seconds, nanoseconds);
}

uint64_t UnixTime::toWindowsFileTime64() const PRIME_NOEXCEPT
{
    Seconds s = _seconds * oneE7;

    Seconds ns = (Seconds)_nanoseconds / 100;

    s += ns;
    s += INT64_C(116444736000000000);

    return (uint64_t)s;
}

UnixTime UnixTime::operator-(const NanosecondTime& rhs) const PRIME_NOEXCEPT
{
    return UnixTime(NanosecondTime::operator-(rhs));
}

UnixTime UnixTime::operator+(const NanosecondTime& rhs) const PRIME_NOEXCEPT
{
    return UnixTime(NanosecondTime::operator+(rhs));
}

UnixTime& UnixTime::operator+=(const NanosecondTime& rhs) PRIME_NOEXCEPT
{
    *this = operator+(rhs);
    return *this;
}

UnixTime& UnixTime::operator-=(const NanosecondTime& rhs) PRIME_NOEXCEPT
{
    *this = operator-(rhs);
    return *this;
}
}
