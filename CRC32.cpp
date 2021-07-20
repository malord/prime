// Copyright 2000-2021 Mark H. P. Lord

#include "CRC32.h"

namespace Prime {

uint32_t CRC32::compute(const void* memory, size_t length) PRIME_NOEXCEPT
{
    CRC32 hasher;
    hasher.process(memory, length);
    return hasher.get();
}

void CRC32::process(const void* memory, size_t length) PRIME_NOEXCEPT
{
    const uint32_t* table = getTable();

    uint32_t crc = _crc ^ UINT32_C(0xffffffff);

    const uint8_t* ptr = (const uint8_t*)memory;
    const uint8_t* end = ptr + length;

    for (; ptr != end; ++ptr) {
        crc = table[(crc ^ *ptr) & 0xff] ^ (crc >> 8);
    }

    _crc = crc ^ UINT32_C(0xffffffff);
}

const uint32_t* CRC32::getTable() PRIME_NOEXCEPT
{
    static const uint32_t* table = NULL;
    if (!table) {
        table = buildTable();
    }
    return table;
}

const uint32_t* CRC32::buildTable() PRIME_NOEXCEPT
{
    static uint32_t table[256];

    for (unsigned int n = 0; n != 256; ++n) {
        uint32_t c = n;

        for (unsigned int k = 0; k != 8; ++k) {
            if (c & 1) {
                c = UINT32_C(0xedb88320) ^ (c >> 1);
            } else {
                c >>= 1;
            }
        }

        table[n] = c;
    }

    return table;
}

Array<uint8_t, 4> CRC32::getBytes() const PRIME_NOEXCEPT
{
    Array<uint8_t, 4> a;
    uint32_t crc = get();
    a[0] = (uint8_t)(crc >> 24);
    a[1] = (uint8_t)(crc >> 16);
    a[2] = (uint8_t)(crc >> 8);
    a[3] = (uint8_t)(crc >> 0);
    return a;
}
}
