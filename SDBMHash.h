// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SDBMHASH_H
#define PRIME_SDBMHASH_H

#include "Config.h"

namespace Prime {

/// Computes a hash using the sdbm algorithm, which is a reasonable general purpose hash algorithm.
/// See http://www.cse.yorku.ca/~oz/hash.html
class SDBMHash {
public:
    typedef uint32_t Result;

    /// Compute the SDBMHash for an array of bytes.
    static uint32_t compute(const void* memory, size_t length, uint32_t initHash = 0) PRIME_NOEXCEPT
    {
        SDBMHash hasher(initHash);
        hasher.process(memory, length);
        return hasher.get();
    }

    explicit SDBMHash(uint32_t initHash = 0) PRIME_NOEXCEPT : _hash(initHash)
    {
    }

    void reset(uint32_t to = 0) PRIME_NOEXCEPT { _hash = to; }

    /// Update the hash with an array of bytes.
    void process(const void* memory, size_t length) PRIME_NOEXCEPT
    {
        uint32_t newHash = _hash;

        const uint8_t* ptr = (const uint8_t*)memory;
        const uint8_t* end = ptr + length;
        for (; ptr != end; ++ptr) {
            newHash = *ptr + (newHash << 6) + (newHash << 16) - newHash;
        }

        _hash = newHash;
    }

    /// Get the current hash value.
    uint32_t get() const PRIME_NOEXCEPT { return _hash; }

private:
    uint32_t _hash;
};
}

#endif
