// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MERSENNETWISTER_H
#define PRIME_MERSENNETWISTER_H

#include "RNGBase.h"

namespace Prime {

/// Mersenne Twister random number generator with improved initialization
/// (http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/emt19937ar.html).
class MersenneTwister : public RNGBase<MersenneTwister> {
public:
    typedef uint32_t Result;
    typedef uint32_t Seed;

    MersenneTwister() PRIME_NOEXCEPT
    {
        _index = N + 1;
        seed(0x4c4d);
    }

    void seed(Seed seed) PRIME_NOEXCEPT
    {
        _state[0] = seed & 0xffffffff;
        for (_index = 1; _index < N; _index++) {
            _state[_index] = (1812433253 * (_state[_index - 1] ^ (_state[_index - 1] >> 30)) + (unsigned int)_index);
            _state[_index] &= 0xffffffff;
        }
    }

    Result generate() PRIME_NOEXCEPT
    {
        uint32_t y;
        static const uint32_t mag01[2] = { 0x0, MATRIX_A };

        if (_index >= N) {
            int kk;

            if (_index == N + 1) {
                seed(5489);
            }

            for (kk = 0; kk < N - M; kk++) {
                y = (_state[kk] & UPPER_MASK) | (_state[kk + 1] & LOWER_MASK);
                _state[kk] = _state[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
            }
            for (; kk < N - 1; kk++) {
                y = (_state[kk] & UPPER_MASK) | (_state[kk + 1] & LOWER_MASK);
                _state[kk] = _state[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
            }
            y = (_state[N - 1] & UPPER_MASK) | (_state[0] & LOWER_MASK);
            _state[N - 1] = _state[M - 1] ^ (y >> 1) ^ mag01[y & 0x1];

            _index = 0;
        }

        y = _state[_index++];

        y ^= (y >> 11);
        y ^= (y << 7) & 0x9d2c5680;
        y ^= (y << 15) & 0xefc60000;
        y ^= (y >> 18);

        return y;
    }

private:
    PRIME_STATIC_CONST(int, N, 624);
    PRIME_STATIC_CONST(int, M, 397);
    PRIME_STATIC_CONST(uint32_t, MATRIX_A, UINT32_C(0x9908b0df));
    PRIME_STATIC_CONST(uint32_t, UPPER_MASK, UINT32_C(0x80000000));
    PRIME_STATIC_CONST(uint32_t, LOWER_MASK, UINT32_C(0x7fffffff));

    uint32_t _state[N];
    int _index;
};
}

#endif
