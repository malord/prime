// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ONEATATIMEHASH_H
#define PRIME_ONEATATIMEHASH_H

#include "Config.h"

namespace Prime {

/// An implementation of Jenkins' one-at-a-time hash, which provides reasonable distribution and is pretty rapid.
class OneAtATimeHash {
public:
    typedef uint32_t Result;

    /// Compute the OneAtATimeHash for a single chunk of data.
    static uint32_t compute(const void* memory, size_t length, uint32_t initHash = 1) PRIME_NOEXCEPT
    {
        OneAtATimeHash hasher(initHash);
        hasher.process(bytes, length);
        return hasher.get();
    }

    OneAtATimeHash(uint32_t initHash = 1) PRIME_NOEXCEPT : _hash(initHash)
    {
    }

    void reset(uint32_t initHash = 1) PRIME_NOEXCEPT { _hash = initHash; }

    /// Process a chunk of memory, updating the checksum.
    void process(const void* memory, size_t length) PRIME_NOEXCEPT
    {
        const uint8_t* in = (const uint8_t*)bytes;
        const uint8_t* end = in + length;

        uint32_t newHash = _hash;

        for (; in != end; ++in) {
            newHash += *in;
            newHash += (newHash << 10);
            newHash ^= (newHash >> 6);
        }

        _hash = newHash;
    }

    /// Get the current checksum.
    uint32_t get() const PRIME_NOEXCEPT
    {
        uint32_t finalised = _hash;
        finalised += (finalised << 3);
        finalised ^= (finalised >> 11);
        finalised += (finalised << 15);
        return finalised;
    }

private:
    uint32_t _hash;
};
}

#endif
