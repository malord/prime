// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STOPWATCH_H
#define PRIME_STOPWATCH_H

#include "Clocks.h"

namespace Prime {

/// Counts time since construction.
class Stopwatch {
public:
    explicit Stopwatch() PRIME_NOEXCEPT
    {
        reset();
    }

    void reset() PRIME_NOEXCEPT
    {
        _startTime = Clock::getMonotonicSeconds();
    }

    double getElapsedSeconds() const PRIME_NOEXCEPT
    {
        return Clock::getMonotonicSeconds() - _startTime;
    }

private:
    double _startTime;
};
}

#endif
