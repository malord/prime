// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DJB2HASH_H
#define PRIME_DJB2HASH_H

#include "Config.h"

namespace Prime {

/// Computes a hash using the djb2 algorithm, which is pretty good for Latin-1 strings.
/// See http://www.cse.yorku.ca/~oz/hash.html
class DJB2Hash {
public:
    typedef uint32_t Result;

    PRIME_STATIC_CONST(unsigned int, defaultHashInit, 5381);

    /// Compute the DJB2Hash for an array of bytes.
    static uint32_t compute(const void* memory, size_t length, uint32_t hashInit = defaultHashInit)
    {
        DJB2Hash hasher(hashInit);
        hasher.process(memory, length);
        return hasher.get();
    }

    explicit DJB2Hash(uint32_t hashInit = defaultHashInit)
        : _hash(hashInit)
    {
    }

    void reset(uint32_t to = defaultHashInit) { _hash = to; }

    /// Update the hash with an array of bytes.
    void process(const void* memory, size_t length) PRIME_NOEXCEPT
    {
        uint32_t newHash = _hash;

        const uint8_t* ptr = (const uint8_t*)memory;
        const uint8_t* end = ptr + length;
        for (; ptr != end; ++ptr) {
            newHash = ((newHash << 5) + newHash) ^ *ptr; // hash * 33 ^ byte
        }

        _hash = newHash;
    }

    /// Get the current hash value.
    uint32_t get() const { return _hash; }

private:
    uint32_t _hash;
};
}

#endif
