// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TIMEOUT_H
#define PRIME_TIMEOUT_H

#include "Clocks.h"
#include <algorithm>

namespace Prime {

/// Keeps track of a timeout.
class Timeout {
public:
    explicit Timeout() PRIME_NOEXCEPT : _endTime(0)
    {
    }

    /// If milliseconds is negative, the timer never expires.
    explicit Timeout(int64_t milliseconds) PRIME_NOEXCEPT
    {
        set(milliseconds);
    }

    /// Initialise a timeout to occur after the specified number of milliseconds. If milliseconds is negative,
    /// the timeout will never expire.
    void set(int64_t milliseconds) PRIME_NOEXCEPT
    {
        if (milliseconds < 0) {
            _endTime = UINT64_MAX;
        } else {
            _endTime = Clock::getMonotonicMilliseconds64() + (uint64_t)milliseconds;
        }
    }

    /// Clamps to INT_MAX if > INT_MAX milliseconds remain, returns < 0 if no timeout is set.
    int getMillisecondsRemaining() const PRIME_NOEXCEPT
    {
        if (_endTime == UINT64_MAX) {
            return -1;
        }

        uint64_t now = Clock::getMonotonicMilliseconds64();
        if (now >= _endTime) {
            return 0;
        }

        uint64_t remaining = _endTime - now;
        if (remaining >= (uint64_t)INT_MAX) {
            return INT_MAX;
        }

        return (int)remaining;
    }

    /// Returns INFINITE (UINT32_MAX) if no timeout was set, otherwise clamps to UINT32_MAX - 1 if >= UINT32_MAX
    /// milliseconds remain.
    uint32_t getWindowsMillisecondsRemaining() const PRIME_NOEXCEPT
    {
        if (_endTime == UINT64_MAX) {
            return UINT32_MAX;
        }

        uint64_t now = Clock::getMonotonicMilliseconds64();
        if (now >= _endTime) {
            return 0;
        }

        uint64_t remaining = _endTime - now;
        if (remaining >= UINT32_MAX) {
            return UINT32_MAX - UINT32_C(1);
        }

        return (uint32_t)remaining;
    }

    bool isExpired() const PRIME_NOEXCEPT
    {
        return Clock::getMonotonicMilliseconds64() >= _endTime;
    }

private:
    uint64_t _endTime;
};
}

#endif
