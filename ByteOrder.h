// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_BYTEORDER_H
#define PRIME_BYTEORDER_H

#include "Config.h"

namespace Prime {

//
// 64-bit integer byte order
//

#ifndef PRIME_NO_INT64

inline uint64_t Make64(uint32_t low, uint32_t high) PRIME_NOEXCEPT
{
    return PRIME_MAKE64(low, high);
}
inline uint64_t Make64(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h) PRIME_NOEXCEPT
{
    return PRIME_MAKE64_BYTES(a, b, c, d, e, f, g, h);
}
inline uint8_t Byte64(uint64_t n, unsigned int b) PRIME_NOEXCEPT
{
    return PRIME_BYTE64(n, b);
}
inline uint32_t Low32(uint64_t n) PRIME_NOEXCEPT
{
    return PRIME_LOW32(n);
}
inline uint32_t High32(uint64_t n) PRIME_NOEXCEPT
{
    return PRIME_HIGH32(n);
}

inline uint64_t Read64LE(const void* ptr) PRIME_NOEXCEPT
{
    return PRIME_READ64LE(ptr);
}
inline uint64_t Read64BE(const void* ptr) PRIME_NOEXCEPT
{
    return PRIME_READ64BE(ptr);
}
inline void Read64LE(const void* ptr, uint64_t& dest) PRIME_NOEXCEPT
{
    dest = PRIME_READ64LE(ptr);
}
inline void Read64BE(const void* ptr, uint64_t& dest) PRIME_NOEXCEPT
{
    dest = PRIME_READ64BE(ptr);
}
inline void Write64LE(void* ptr, uint64_t n) PRIME_NOEXCEPT
{
    PRIME_WRITE64LE(ptr, n);
}
inline void Write64BE(void* ptr, uint64_t n) PRIME_NOEXCEPT
{
    PRIME_WRITE64BE(ptr, n);
}
inline void Write64(void* ptr, uint64_t n) PRIME_NOEXCEPT
{
    PRIME_WRITE64(ptr, n);
}
inline uint64_t Read64(const void* ptr) PRIME_NOEXCEPT
{
    return PRIME_READ64(ptr);
}

inline uint64_t Swap64(uint64_t n) PRIME_NOEXCEPT
{
    return PRIME_SWAP64(n);
}
inline void Swap64(void* ptr) PRIME_NOEXCEPT
{
    PRIME_SWAP64_IN_PLACE(ptr);
}
inline void Swap64Array(uint64_t* array, size_t count) PRIME_NOEXCEPT
{
    for (; count; --count, ++array) {
        PRIME_SWAP64_IN_PLACE(array);
    }
}
inline uint64_t Swap64LE(uint64_t n) PRIME_NOEXCEPT
{
    return PRIME_SWAP64LE(n);
}
inline uint64_t Swap64BE(uint64_t n) PRIME_NOEXCEPT
{
    return PRIME_SWAP64BE(n);
}

#endif // PRIME_NO_INT64

//
// 32-bit integer byte order
//

inline uint32_t Make32(uint16_t low, uint16_t high) PRIME_NOEXCEPT
{
    return PRIME_MAKE32(low, high);
}
inline uint32_t Make32(uint8_t lowest, uint8_t low, uint8_t high, uint8_t highest) PRIME_NOEXCEPT
{
    return PRIME_MAKE32_BYTES(lowest, low, high, highest);
}
inline uint8_t Byte32(uint32_t n, unsigned int b) PRIME_NOEXCEPT
{
    return PRIME_BYTE32(n, b);
}
inline uint16_t Low16(uint32_t n) PRIME_NOEXCEPT
{
    return PRIME_LOW16(n);
}
inline uint16_t High16(uint32_t n) PRIME_NOEXCEPT
{
    return PRIME_HIGH16(n);
}

inline uint32_t Read32LE(const void* ptr) PRIME_NOEXCEPT
{
    return PRIME_READ32LE(ptr);
}
inline uint32_t Read32BE(const void* ptr) PRIME_NOEXCEPT
{
    return PRIME_READ32BE(ptr);
}
inline void Read32LE(const void* ptr, uint32_t& dest) PRIME_NOEXCEPT
{
    dest = PRIME_READ32LE(ptr);
}
inline void Read32BE(const void* ptr, uint32_t& dest) PRIME_NOEXCEPT
{
    dest = PRIME_READ32BE(ptr);
}
inline void Write32LE(void* ptr, uint32_t n) PRIME_NOEXCEPT
{
    PRIME_WRITE32LE(ptr, n);
}
inline void Write32BE(void* ptr, uint32_t n) PRIME_NOEXCEPT
{
    PRIME_WRITE32BE(ptr, n);
}
inline void Write32(void* ptr, uint32_t n) PRIME_NOEXCEPT
{
    PRIME_WRITE32(ptr, n);
}
inline uint32_t Read32(const void* ptr) PRIME_NOEXCEPT
{
    return PRIME_READ32(ptr);
}

inline uint32_t Swap32(uint32_t n) PRIME_NOEXCEPT
{
    return PRIME_SWAP32(n);
}
inline void Swap32(void* ptr) PRIME_NOEXCEPT
{
    PRIME_SWAP32_IN_PLACE(ptr);
}
inline void Swap32Array(uint32_t* array, size_t count) PRIME_NOEXCEPT
{
    for (; count; --count, ++array) {
        PRIME_SWAP32_IN_PLACE(array);
    }
}
inline uint32_t Swap32LE(uint32_t n) PRIME_NOEXCEPT
{
    return PRIME_SWAP32LE(n);
}
inline uint32_t Swap32BE(uint32_t n) PRIME_NOEXCEPT
{
    return PRIME_SWAP32BE(n);
}

//
// 16-bit integer byte order
//

inline uint16_t Make16(uint8_t low, uint8_t high) PRIME_NOEXCEPT
{
    return PRIME_MAKE16_BYTES(low, high);
}
inline uint8_t Low8(uint16_t n) PRIME_NOEXCEPT
{
    return PRIME_LOW8(n);
}
inline uint8_t Low8(unsigned int n) PRIME_NOEXCEPT
{
    return PRIME_LOW8(n);
}
inline uint8_t High8(uint16_t n) PRIME_NOEXCEPT
{
    return PRIME_HIGH8(n);
}
inline uint8_t High8(unsigned int n) PRIME_NOEXCEPT
{
    return PRIME_HIGH8(n);
}

inline uint16_t Read16LE(const void* ptr) PRIME_NOEXCEPT
{
    return PRIME_READ16LE(ptr);
}
inline uint16_t Read16BE(const void* ptr) PRIME_NOEXCEPT
{
    return PRIME_READ16BE(ptr);
}
inline void Read16LE(const void* ptr, uint16_t& dest) PRIME_NOEXCEPT
{
    dest = PRIME_READ16LE(ptr);
}
inline void Read16BE(const void* ptr, uint16_t& dest) PRIME_NOEXCEPT
{
    dest = PRIME_READ16BE(ptr);
}
inline void Write16LE(void* ptr, uint16_t n) PRIME_NOEXCEPT
{
    PRIME_WRITE16LE(ptr, n);
}
inline void Write16BE(void* ptr, uint16_t n) PRIME_NOEXCEPT
{
    PRIME_WRITE16BE(ptr, n);
}
inline void Write16(void* ptr, uint16_t n) PRIME_NOEXCEPT
{
    PRIME_WRITE16(ptr, n);
}
inline uint16_t Read16(const void* ptr) PRIME_NOEXCEPT
{
    return PRIME_READ16(ptr);
}

inline uint16_t Swap16(uint16_t n) PRIME_NOEXCEPT
{
    return PRIME_SWAP16(n);
}
inline void Swap16(void* ptr) PRIME_NOEXCEPT
{
    PRIME_SWAP16_IN_PLACE(ptr);
}
inline void Swap16Array(uint16_t* array, size_t count) PRIME_NOEXCEPT
{
    for (; count; --count, ++array) {
        PRIME_SWAP16_IN_PLACE(array);
    }
}
inline uint16_t Swap16LE(uint16_t n) PRIME_NOEXCEPT
{
    return PRIME_SWAP16LE(n);
}
inline uint16_t Swap16BE(uint16_t n) PRIME_NOEXCEPT
{
    return PRIME_SWAP16BE(n);
}

//
// Float32 byte order
//

typedef PrimeFloatInt32 FloatInt32;

inline void WriteFloat32(void* ptr, Float32 value) PRIME_NOEXCEPT
{
    FloatInt32 u;
    u.f = value;
    PRIME_WRITE32(ptr, u.u);
}

inline Float32 ReadFloat32(const void* ptr) PRIME_NOEXCEPT
{
    FloatInt32 u;
    u.u = PRIME_READ32(ptr);
    return u.f;
}

inline Float32 ReadFloat32LE(const void* ptr) PRIME_NOEXCEPT
{
    FloatInt32 u;
    u.u = Read32LE(ptr);
    return u.f;
}

inline Float32 ReadFloat32BE(const void* ptr) PRIME_NOEXCEPT
{
    FloatInt32 u;
    u.u = Read32BE(ptr);
    return u.f;
}

inline void ReadFloat32LE(const void* ptr, Float32& value) PRIME_NOEXCEPT
{
    value = ReadFloat32LE(ptr);
}
inline void ReadFloat32BE(const void* ptr, Float32& value) PRIME_NOEXCEPT
{
    value = ReadFloat32BE(ptr);
}

inline void WriteFloat32LE(void* ptr, Float32 n) PRIME_NOEXCEPT
{
    FloatInt32 u;
    u.f = n;
    Write32LE(ptr, u.u);
}

inline void WriteFloat32BE(void* ptr, Float32 n) PRIME_NOEXCEPT
{
    FloatInt32 u;
    u.f = n;
    Write32BE(ptr, u.u);
}

inline void SwapFloat32(void* ptr) PRIME_NOEXCEPT
{
    PRIME_SWAP_FLOAT32_IN_PLACE(ptr);
}
inline void SwapFloat32Array(Float32* array, size_t count) PRIME_NOEXCEPT
{
    for (; count; --count, ++array) {
        PRIME_SWAP_FLOAT32_IN_PLACE(array);
    }
}

//
// Float64 byte order
//

#if !defined(PRIME_NO_INT64) && !defined(PRIME_NO_FLOAT64)

typedef PrimeFloatInt64 FloatInt64;

inline void WriteFloat64(void* ptr, Float64 value) PRIME_NOEXCEPT
{
    FloatInt64 u;
    u.f = value;
    PRIME_WRITE64(ptr, u.u);
}

inline Float64 ReadFloat64(const void* ptr) PRIME_NOEXCEPT
{
    FloatInt64 u;
    u.u = PRIME_READ64(ptr);
    return u.f;
}

inline Float64 ReadFloat64LE(const void* ptr) PRIME_NOEXCEPT
{
    FloatInt64 u;
    u.u = Read64LE(ptr);
    return u.f;
}

inline Float64 ReadFloat64BE(const void* ptr) PRIME_NOEXCEPT
{
    FloatInt64 u;
    u.u = Read64BE(ptr);
    return u.f;
}

inline void ReadFloat64LE(const void* ptr, Float64& value) PRIME_NOEXCEPT
{
    value = ReadFloat64LE(ptr);
}
inline void ReadFloat64BE(const void* ptr, Float64& value) PRIME_NOEXCEPT
{
    value = ReadFloat64BE(ptr);
}

inline void WriteFloat64LE(void* ptr, Float64 n) PRIME_NOEXCEPT
{
    FloatInt64 u;
    u.f = n;
    Write64LE(ptr, u.u);
}

inline void WriteFloat64BE(void* ptr, Float64 n) PRIME_NOEXCEPT
{
    FloatInt64 u;
    u.f = n;
    Write64BE(ptr, u.u);
}

inline void SwapFloat64(void* ptr) PRIME_NOEXCEPT
{
    PRIME_SWAP_FLOAT64_IN_PLACE(ptr);
}

inline void SwapFloat64Array(Float64* array, size_t count) PRIME_NOEXCEPT
{
    for (; count; --count, ++array) {
        PRIME_SWAP_FLOAT64_IN_PLACE(array);
    }
}

#endif // PRIME_NO_FLOAT64

}

#endif
