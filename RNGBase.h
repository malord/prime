// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_RNGBASE_H
#define PRIME_RNGBASE_H

#include "Log.h"
#include <limits>

namespace Prime {

/// Base class (recursive template) for random number generators.
template <typename RNG>
class RNGBase {
public:
    template <typename Seed>
    static RNG createWithSeed(Seed seedValue) PRIME_NOEXCEPT
    {
        RNG rng;
        rng.seed(seedValue);
        return rng;
    }

    float generateFloat() PRIME_NOEXCEPT
    {
        // A float can't accurately hold the maximum value of most RNGs (UINT32_MAX), so do the maths as a double.
        return (float)this->generateDouble();
    }

    double generateDouble() PRIME_NOEXCEPT
    {
        return (double)getRNG()->generate() / std::numeric_limits<typename RNG::Result>::max();
    }

    float generateSignedFloat() PRIME_NOEXCEPT
    {
        return getRNG()->generateFloat() * 2.0f - 1.0f;
    }

    double generateSignedDouble() PRIME_NOEXCEPT
    {
        return getRNG()->generateDouble() * 2.0 - 1.0;
    }

    bool generateBool() PRIME_NOEXCEPT
    {
        return !(getRNG()->generate() & 1);
    }

    bool generateBytes(void* buffer, size_t bytesNeeded, Log*)
    {
        uint8_t* ptr = (uint8_t*)buffer;
        uint8_t* end = ptr + bytesNeeded;

        for (; ptr != end; ++ptr) {
            *ptr = (uint8_t)getRNG()->generate();
        }
        return true;
    }

private:
    RNG* getRNG() PRIME_NOEXCEPT { return static_cast<RNG*>(this); }

    const RNG* getRNG() const PRIME_NOEXCEPT { return static_cast<const RNG*>(this); }
};
}

#endif
