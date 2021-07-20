// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SHA256_H
#define PRIME_SHA256_H

#include "Array.h"
#include "Config.h"

namespace Prime {

/// Computes SHA256 hashes.
class PRIME_PUBLIC SHA256 {
public:
    enum { digestSize = 32u };
    enum { blockSize = 64u };

    typedef Array<uint8_t, digestSize> Result;

    /// Compute the SHA256 for a single chunk of data.
    static Result compute(const void* memory, size_t length) PRIME_NOEXCEPT;

    SHA256()
    PRIME_NOEXCEPT { reset(); }

    SHA256(const SHA256& copy)
    PRIME_NOEXCEPT { operator=(copy); }

    SHA256& operator=(const SHA256& copy) PRIME_NOEXCEPT;

    /// Restart the computation.
    void reset() PRIME_NOEXCEPT;

    /// Process a chunk of memory, updating the checksum.
    void process(const void* memory, size_t length) PRIME_NOEXCEPT;

    /// Get the current checksum.
    Result get() const PRIME_NOEXCEPT;

    Result getBytes() const PRIME_NOEXCEPT { return get(); }

private:
    struct Block {
        uint8_t bytes[blockSize];
        uint8_t* ptr;

        unsigned int getLength() const PRIME_NOEXCEPT { return (unsigned int)(ptr - bytes); }

        Block& operator=(const Block& copy) PRIME_NOEXCEPT;

        Block() PRIME_NOEXCEPT { ptr = bytes; }

        Block(const Block& copy) PRIME_NOEXCEPT { operator=(copy); }
    } _block;

    struct State {
        uint32_t blockCount;
        uint32_t hash[8];
    } _state;

    static void processBlock(State& state, const uint8_t* bytes) PRIME_NOEXCEPT;
};
}

#endif
