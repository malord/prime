// Copyright 2000-2021 Mark H. P. Lord

#include "MD5.h"
#include "NumberUtils.h"
#include <string.h>

namespace Prime {

//
// MD5::Block
//

MD5::Block& MD5::Block::operator=(const Block& copy) PRIME_NOEXCEPT
{
    memcpy(bytes, copy.bytes, sizeof(bytes));
    ptr = bytes + (copy.ptr - copy.bytes);
    return *this;
}

//
// MD5
//

MD5::Result MD5::compute(const void* memory, size_t length) PRIME_NOEXCEPT
{
    MD5 hasher;
    hasher.process(memory, length);
    return hasher.get();
}

MD5& MD5::operator=(const MD5& copy) PRIME_NOEXCEPT
{
    _state = copy._state;
    _block = copy._block;
    return *this;
}

void MD5::reset() PRIME_NOEXCEPT
{
    _state.blockCount = 0;
    _state.hash[0] = 0x67452301;
    _state.hash[1] = 0xefcdab89;
    _state.hash[2] = 0x98badcfe;
    _state.hash[3] = 0x10325476;

    _block.ptr = _block.bytes;
}

void MD5::process(const void* memory, size_t length) PRIME_NOEXCEPT
{
    while (length) {
        size_t space = blockSize - (_block.ptr - _block.bytes);
        if (!space) {
            processBlock(_state, _block.bytes);
            _block.ptr = _block.bytes;
            space = blockSize;
        }

        size_t thisTime = Min(space, length);

        memcpy(_block.ptr, memory, thisTime);
        _block.ptr += thisTime;
        length -= thisTime;
        memory = (char*)memory + thisTime;
    }
}

MD5::Result MD5::get() const PRIME_NOEXCEPT
{
    State state = _state;
    Block block = _block;

    if (block.getLength() == blockSize) {
        processBlock(state, block.bytes);
        block.ptr = block.bytes;
    }

    uint32_t blockLength = block.getLength();
    uint64_t messageLength = (uint64_t)state.blockCount * blockSize + blockLength;

    uint8_t* bytes = block.bytes;
    bytes[blockLength++] = 0x80;
    if (blockLength <= 56) {
        memset(bytes + blockLength, 0, blockSize - blockLength - 5);
    } else {
        memset(bytes + blockLength, 0, blockSize - blockLength);
        processBlock(state, block.bytes);
        memset(bytes, 0, 56);
    }

    uint64_t messageLengthBits = messageLength << 3;
    bytes[56] = (uint8_t)messageLengthBits;
    bytes[57] = (uint8_t)(messageLengthBits >> 8);
    bytes[58] = (uint8_t)(messageLengthBits >> 16);
    bytes[59] = (uint8_t)(messageLengthBits >> 24);
    bytes[60] = (uint8_t)((messageLength >> 29) & 0x07);
    bytes[61] = bytes[62] = bytes[63] = 0;

    processBlock(state, block.bytes);

    Result digest;

    uint32_t* hash = state.hash;
    for (unsigned int i = 0; i != digestSize >> 2; i++) {
        digest[i * 4] = (uint8_t)(hash[i] >> 0);
        digest[i * 4 + 1] = (uint8_t)(hash[i] >> 8);
        digest[i * 4 + 2] = (uint8_t)(hash[i] >> 16);
        digest[i * 4 + 3] = (uint8_t)(hash[i] >> 24);
    }

    return digest;
}

void MD5::processBlock(State& state, const uint8_t* bytes) PRIME_NOEXCEPT
{
    static const unsigned char shifts[64] = {
        7,
        12,
        17,
        22,
        7,
        12,
        17,
        22,
        7,
        12,
        17,
        22,
        7,
        12,
        17,
        22,
        5,
        9,
        14,
        20,
        5,
        9,
        14,
        20,
        5,
        9,
        14,
        20,
        5,
        9,
        14,
        20,
        4,
        11,
        16,
        23,
        4,
        11,
        16,
        23,
        4,
        11,
        16,
        23,
        4,
        11,
        16,
        23,
        6,
        10,
        15,
        21,
        6,
        10,
        15,
        21,
        6,
        10,
        15,
        21,
        6,
        10,
        15,
        21,
    };

    static const uint32_t constants[64] = {
        0xd76aa478,
        0xe8c7b756,
        0x242070db,
        0xc1bdceee,
        0xf57c0faf,
        0x4787c62a,
        0xa8304613,
        0xfd469501,
        0x698098d8,
        0x8b44f7af,
        0xffff5bb1,
        0x895cd7be,
        0x6b901122,
        0xfd987193,
        0xa679438e,
        0x49b40821,
        0xf61e2562,
        0xc040b340,
        0x265e5a51,
        0xe9b6c7aa,
        0xd62f105d,
        0x02441453,
        0xd8a1e681,
        0xe7d3fbc8,
        0x21e1cde6,
        0xc33707d6,
        0xf4d50d87,
        0x455a14ed,
        0xa9e3e905,
        0xfcefa3f8,
        0x676f02d9,
        0x8d2a4c8a,
        0xfffa3942,
        0x8771f681,
        0x6d9d6122,
        0xfde5380c,
        0xa4beea44,
        0x4bdecfa9,
        0xf6bb4b60,
        0xbebfbc70,
        0x289b7ec6,
        0xeaa127fa,
        0xd4ef3085,
        0x04881d05,
        0xd9d4d039,
        0xe6db99e5,
        0x1fa27cf8,
        0xc4ac5665,
        0xf4292244,
        0x432aff97,
        0xab9423a7,
        0xfc93a039,
        0x655b59c3,
        0x8f0ccc92,
        0xffeff47d,
        0x85845dd1,
        0x6fa87e4f,
        0xfe2ce6e0,
        0xa3014314,
        0x4e0811a1,
        0xf7537e82,
        0xbd3af235,
        0x2ad7d2bb,
        0xeb86d391,
    };

    state.blockCount++;

    uint32_t a = state.hash[0], b = state.hash[1], c = state.hash[2], d = state.hash[3];

    for (unsigned int i = 0; i != 64; i++) {
        uint32_t f;
        unsigned int g;
        if (i < 16) {
            f = (b & c) | (~b & d);
            g = i;
        } else if (i < 32) {
            f = (d & b) | (~d & c);
            g = (5 * i + 1) & 0x0f;
        } else if (i < 48) {
            f = b ^ c ^ d;
            g = (3 * i + 5) & 0x0f;
        } else {
            f = c ^ (b | ~d);
            g = (7 * i) & 0x0f;
        }

        uint32_t dTemp = d;
        d = c;
        c = b;
        const uint8_t* bytesG = bytes + g * 4;
        uint32_t word = bytesG[0] | (bytesG[1] << 8) | (bytesG[2] << 16) | (bytesG[3] << 24);
        b += LeftRotate32(a + f + constants[i] + word, shifts[i]);
        a = dTemp;
    }

    state.hash[0] += a;
    state.hash[1] += b;
    state.hash[2] += c;
    state.hash[3] += d;
}
}
