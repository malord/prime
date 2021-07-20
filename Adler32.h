// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ADLER32_H
#define PRIME_ADLER32_H

#include "Config.h"

namespace Prime {

/// Computes Adler-32 checksums. Very poor as a hash function for short strings.
class Adler32 {
public:
    typedef uint32_t Result;

    PRIME_STATIC_CONST(unsigned int, adlerPrime, 65521u);

    /// Compute the Adler32 for a single chunk of data.
    static uint32_t compute(const void* memory, size_t length) PRIME_NOEXCEPT
    {
        Adler32 adler32;
        adler32.process(memory, length);
        return adler32.get();
    }

    Adler32(uint32_t a = 1, uint32_t b = 0) PRIME_NOEXCEPT : _a(a),
                                                             _b(b)
    {
    }

    /// Restart the computation.
    void reset(uint32_t a = 1, uint32_t b = 0) PRIME_NOEXCEPT
    {
        _a = a;
        _b = b;
    }

    /// Process a chunk of memory, updating the checksum.
    void process(const void* memory, size_t length) PRIME_NOEXCEPT
    {
        const uint8_t* ptr = (const uint8_t*)memory;

        uint32_t a = _a;
        uint32_t b = _b;

        while (length) {
            size_t chunk = length < 5552 ? length : 5552;
            const uint8_t* end = ptr + chunk;
            length -= chunk;

            while (ptr != end) {
                a += *ptr++;
                b += a;
            }

            a %= adlerPrime;
            b %= adlerPrime;
        }

        _a = a;
        _b = b;
    }

    /// Get the current checksum.
    uint32_t get() const PRIME_NOEXCEPT { return (uint32_t)((_b << 16) | _a); }

private:
    uint32_t _a;
    uint32_t _b;
};
}

#endif
