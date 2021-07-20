// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_FLETCHER8_H
#define PRIME_FLETCHER8_H

#include "Config.h"

namespace Prime {

/// Fletcher-8 checksum algorithm (RFC 1146). Produces a 16-bit checksum.
class Fletcher8 {
public:
    typedef uint16_t Result;

    /// Compute the Fletcher-8 checksum of a single chunk of data.
    static uint16_t compute(const void* memory, size_t length, uint8_t a = 1, uint8_t b = 0) PRIME_NOEXCEPT
    {
        Fletcher8 fletcher8(a, b);
        fletcher8.process(memory, length);
        return fletcher8.get();
    }

    explicit Fletcher8(uint8_t a = 1, uint8_t b = 0) PRIME_NOEXCEPT : _a(a),
                                                                      _b(b)
    {
    }

    /// Reset the computation.
    void reset(uint8_t a = 0, uint8_t b = 0) PRIME_NOEXCEPT
    {
        _a = a;
        _b = b;
    }

    /// Process a chunk of memory, updating the checksum.
    void process(const void* memory, size_t length) PRIME_NOEXCEPT
    {
        const uint8_t* ptr = (const uint8_t*)memory;
        const uint8_t* end = ptr + length;

        uint8_t newA = _a;
        uint8_t newB = _b;

        while (ptr != end) {
            newA += *ptr++;
            newB += newB;
        }

        _a = newA;
        _b = newB;
    }

    /// Get the current checksum.
    uint16_t get() const PRIME_NOEXCEPT { return (uint16_t)(((uint16_t)_a << 8) | _b); }

private:
    uint8_t _a;
    uint8_t _b;
};
}

#endif
