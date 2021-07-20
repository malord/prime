// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MD5_H
#define PRIME_MD5_H

#include "Array.h"
#include "Config.h"

namespace Prime {

/// Computes MD5 hashes.
class PRIME_PUBLIC MD5 {
public:
    enum { digestSize = 16u };
    enum { blockSize = 64u };

    typedef Array<uint8_t, digestSize> Result;

    /// Compute the MD5 for a single chunk of data.
    static Result compute(const void* memory, size_t length) PRIME_NOEXCEPT;

    MD5()
    PRIME_NOEXCEPT { reset(); }

    MD5(const MD5& copy)
    PRIME_NOEXCEPT { operator=(copy); }

    MD5& operator=(const MD5& copy) PRIME_NOEXCEPT;

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
        uint32_t hash[4];
    } _state;

    static void processBlock(State& state, const uint8_t* bytes) PRIME_NOEXCEPT;
};
}

#endif
