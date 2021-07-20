// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CRC32_H
#define PRIME_CRC32_H

#include "Array.h"
#include "Config.h"

namespace Prime {

/// Computes CRC-32 checksums (as used in zip files).
class PRIME_PUBLIC CRC32 {
public:
    enum { digestSize = 4 };

    typedef uint32_t Result;

    /// Compute the CRC32 for a single chunk of data.
    static uint32_t compute(const void* memory, size_t length) PRIME_NOEXCEPT;

    explicit CRC32(uint32_t init = defaultInitCRC) PRIME_NOEXCEPT : _crc(init)
    {
    }

    /// Restart the computation.
    void reset(uint32_t init = defaultInitCRC) PRIME_NOEXCEPT { _crc = init; }

    /// Process a chunk of memory, updating the checksum.
    void process(const void* memory, size_t length) PRIME_NOEXCEPT;

    /// Get the current checksum.
    uint32_t get() const PRIME_NOEXCEPT { return _crc; }

    /// Get the current checksum as an array of bytes (big-endian)/
    Array<uint8_t, 4> getBytes() const PRIME_NOEXCEPT;

private:
    static const uint32_t* getTable() PRIME_NOEXCEPT;

    static const uint32_t* buildTable() PRIME_NOEXCEPT;

    uint32_t _crc;

    enum { defaultInitCRC = 0u };
};
}

#endif
