// Copyright 2000-2021 Mark H. P. Lord

#include "SHA1.h"
#include "NumberUtils.h"
#include <string.h>

namespace Prime {

//
// SHA1::Block
//

SHA1::Block& SHA1::Block::operator=(const Block& copy) PRIME_NOEXCEPT
{
    memcpy(bytes, copy.bytes, sizeof(bytes));
    ptr = bytes + (copy.ptr - copy.bytes);
    return *this;
}

//
// SHA1
//

static const uint32_t constant0 = 0x5A827999;
static const uint32_t constant1 = 0x6ED9EBA1;
static const uint32_t constant2 = 0x8F1BBCDC;
static const uint32_t constant3 = 0xCA62C1D6;

SHA1::Result SHA1::compute(const void* memory, size_t length) PRIME_NOEXCEPT
{
    SHA1 hasher;
    hasher.process(memory, length);
    return hasher.get();
}

SHA1& SHA1::operator=(const SHA1& copy) PRIME_NOEXCEPT
{
    _state = copy._state;
    _block = copy._block;
    return *this;
}

void SHA1::reset() PRIME_NOEXCEPT
{
    _state.blockCount = 0;
    _state.hash[0] = 0x67452301;
    _state.hash[1] = 0xefcdab89;
    _state.hash[2] = 0x98badcfe;
    _state.hash[3] = 0x10325476;
    _state.hash[4] = 0xc3d2e1f0;

    _block.ptr = _block.bytes;
}

void SHA1::process(const void* memory, size_t length) PRIME_NOEXCEPT
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

SHA1::Result SHA1::get() const PRIME_NOEXCEPT
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
    bytes[63] = (uint8_t)messageLengthBits;
    bytes[62] = (uint8_t)(messageLengthBits >> 8);
    bytes[61] = (uint8_t)(messageLengthBits >> 16);
    bytes[60] = (uint8_t)(messageLengthBits >> 24);
    bytes[59] = (uint8_t)((messageLength >> 29) & 0x07);
    bytes[58] = bytes[57] = bytes[56] = 0;

    processBlock(state, block.bytes);

    Result digest;

    uint32_t* hash = state.hash;
    for (unsigned int i = 0; i != digestSize >> 2; i++) {
        digest[i * 4 + 3] = (uint8_t)(hash[i] >> 0);
        digest[i * 4 + 2] = (uint8_t)(hash[i] >> 8);
        digest[i * 4 + 1] = (uint8_t)(hash[i] >> 16);
        digest[i * 4] = (uint8_t)(hash[i] >> 24);
    }

    return digest;
}

void SHA1::processBlock(State& state, const uint8_t* bytes) PRIME_NOEXCEPT
{
    uint32_t a = state.hash[0], b = state.hash[1], c = state.hash[2], d = state.hash[3], e = state.hash[4];
    uint32_t w[16], f, a5, b30, t, tmp, j;
    int i;

    state.blockCount++;

    for (i = 0; i < 16; i++) {
        j = i * 4;
        w[i] = bytes[j] << 24 | bytes[j + 1] << 16 | bytes[j + 2] << 8 | bytes[j + 3];
    }

    i = 0;

    for (; i < 16; i++) {
        f = (b & c) | (~b & d);
        a5 = a << 5 | a >> (32 - 5);
        b30 = b << 30 | b >> (32 - 30);
        t = a5 + f + e + w[i & 0xf] + constant0;
        e = d;
        d = c;
        c = b30;
        b = a;
        a = t;
    }

    for (; i < 20; i++) {
        tmp = w[(i - 3) & 0xf] ^ w[(i - 8) & 0xf] ^ w[(i - 14) & 0xf] ^ w[i & 0xf];
        w[i & 0xf] = tmp << 1 | tmp >> (32 - 1);

        f = (b & c) | (~b & d);
        a5 = a << 5 | a >> (32 - 5);
        b30 = b << 30 | b >> (32 - 30);
        t = a5 + f + e + w[i & 0xf] + constant0;
        e = d;
        d = c;
        c = b30;
        b = a;
        a = t;
    }

    for (; i < 40; i++) {
        tmp = w[(i - 3) & 0xf] ^ w[(i - 8) & 0xf] ^ w[(i - 14) & 0xf] ^ w[i & 0xf];
        w[i & 0xf] = tmp << 1 | tmp >> (32 - 1);

        f = b ^ c ^ d;
        a5 = a << 5 | a >> (32 - 5);
        b30 = b << 30 | b >> (32 - 30);
        t = a5 + f + e + w[i & 0xf] + constant1;
        e = d;
        d = c;
        c = b30;
        b = a;
        a = t;
    }

    for (; i < 60; i++) {
        tmp = w[(i - 3) & 0xf] ^ w[(i - 8) & 0xf] ^ w[(i - 14) & 0xf] ^ w[i & 0xf];
        w[i & 0xf] = tmp << 1 | tmp >> (32 - 1);

        f = ((b | c) & d) | (b & c);
        a5 = a << 5 | a >> (32 - 5);
        b30 = b << 30 | b >> (32 - 30);
        t = a5 + f + e + w[i & 0xf] + constant2;
        e = d;
        d = c;
        c = b30;
        b = a;
        a = t;
    }

    for (; i < 80; i++) {
        tmp = w[(i - 3) & 0xf] ^ w[(i - 8) & 0xf] ^ w[(i - 14) & 0xf] ^ w[i & 0xf];
        w[i & 0xf] = tmp << 1 | tmp >> (32 - 1);

        f = b ^ c ^ d;
        a5 = a << 5 | a >> (32 - 5);
        b30 = b << 30 | b >> (32 - 30);
        t = a5 + f + e + w[i & 0xf] + constant3;
        e = d;
        d = c;
        c = b30;
        b = a;
        a = t;
    }

    state.hash[0] += a;
    state.hash[1] += b;
    state.hash[2] += c;
    state.hash[3] += d;
    state.hash[4] += e;
}
}
