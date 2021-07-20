// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PARKMILLERRNG_H
#define PRIME_PARKMILLERRNG_H

#include "RNGBase.h"

namespace Prime {

/// Park-Miller "minimal standard" 31 bit pseudo-random number generator.
/// http://www.firstpr.com.au/dsp/rand31/
class ParkMillerRNG : public RNGBase<ParkMillerRNG> {
public:
    typedef int32_t Result;
    typedef int32_t Seed;

    ParkMillerRNG() PRIME_NOEXCEPT : _seed(1)
    {
    }

    void seed(Seed seed) PRIME_NOEXCEPT { _seed = seed; }

    Seed getSeed() const PRIME_NOEXCEPT { return _seed; }

    Result generate() PRIME_NOEXCEPT
    {
        uint32_t hi, lo;

        _seed = (int32_t)((uint32_t)_seed & UINT32_C(0x7fffffff));

        lo = 16807 * (_seed & 0xffff);
        hi = 16807 * (_seed >> 16);

        lo += (hi & 0x7fff) << 16;
        lo += hi >> 15;

        if (lo > 0x7fffffff) {
            lo -= 0x7fffffff;
        }

        _seed = (int32_t)lo;
        PRIME_DEBUG_ASSERT(_seed >= 0);
        return _seed;
    }

private:
    Seed _seed;
};
}

#endif
