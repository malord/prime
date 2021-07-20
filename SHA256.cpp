// Copyright 2000-2021 Mark H. P. Lord

#include "SHA256.h"
#include "NumberUtils.h"
#include <string.h>

namespace Prime {

//
// SHA256::Block
//

SHA256::Block& SHA256::Block::operator=(const Block& copy) PRIME_NOEXCEPT
{
    memcpy(bytes, copy.bytes, sizeof(bytes));
    ptr = bytes + (copy.ptr - copy.bytes);
    return *this;
}

//
// SHA256
//

static const uint32_t constants[64] = {
    0x428a2f98,
    0x71374491,
    0xb5c0fbcf,
    0xe9b5dba5,
    0x3956c25b,
    0x59f111f1,
    0x923f82a4,
    0xab1c5ed5,
    0xd807aa98,
    0x12835b01,
    0x243185be,
    0x550c7dc3,
    0x72be5d74,
    0x80deb1fe,
    0x9bdc06a7,
    0xc19bf174,
    0xe49b69c1,
    0xefbe4786,
    0x0fc19dc6,
    0x240ca1cc,
    0x2de92c6f,
    0x4a7484aa,
    0x5cb0a9dc,
    0x76f988da,
    0x983e5152,
    0xa831c66d,
    0xb00327c8,
    0xbf597fc7,
    0xc6e00bf3,
    0xd5a79147,
    0x06ca6351,
    0x14292967,
    0x27b70a85,
    0x2e1b2138,
    0x4d2c6dfc,
    0x53380d13,
    0x650a7354,
    0x766a0abb,
    0x81c2c92e,
    0x92722c85,
    0xa2bfe8a1,
    0xa81a664b,
    0xc24b8b70,
    0xc76c51a3,
    0xd192e819,
    0xd6990624,
    0xf40e3585,
    0x106aa070,
    0x19a4c116,
    0x1e376c08,
    0x2748774c,
    0x34b0bcb5,
    0x391c0cb3,
    0x4ed8aa4a,
    0x5b9cca4f,
    0x682e6ff3,
    0x748f82ee,
    0x78a5636f,
    0x84c87814,
    0x8cc70208,
    0x90befffa,
    0xa4506ceb,
    0xbef9a3f7,
    0xc67178f2,
};

SHA256::Result SHA256::compute(const void* memory, size_t length) PRIME_NOEXCEPT
{
    SHA256 hasher;
    hasher.process(memory, length);
    return hasher.get();
}

SHA256& SHA256::operator=(const SHA256& copy) PRIME_NOEXCEPT
{
    _state = copy._state;
    _block = copy._block;
    return *this;
}

void SHA256::reset() PRIME_NOEXCEPT
{
    _state.blockCount = 0;
    _state.hash[0] = 0x6a09e667;
    _state.hash[1] = 0xbb67ae85;
    _state.hash[2] = 0x3c6ef372;
    _state.hash[3] = 0xa54ff53a;
    _state.hash[4] = 0x510e527f;
    _state.hash[5] = 0x9b05688c;
    _state.hash[6] = 0x1f83d9ab;
    _state.hash[7] = 0x5be0cd19;

    _block.ptr = _block.bytes;
}

void SHA256::process(const void* memory, size_t length) PRIME_NOEXCEPT
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

SHA256::Result SHA256::get() const PRIME_NOEXCEPT
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

void SHA256::processBlock(State& state, const uint8_t* bytes) PRIME_NOEXCEPT
{
    uint32_t a = state.hash[0], b = state.hash[1], c = state.hash[2], d = state.hash[3],
             e = state.hash[4], f = state.hash[5], g = state.hash[6], h = state.hash[7];

    state.blockCount++;

    int i;
    uint32_t w[64];
    for (i = 0; i != 16; ++i) {
        w[i] = bytes[4 * i] << 24 | bytes[4 * i + 1] << 16 | bytes[4 * i + 2] << 8 | bytes[4 * i + 3];
    }

    uint32_t v1, t1, v2, t2;

    for (; i < 64; i++) {
        v1 = w[i - 2];
        t1 = RightRotate32(v1, 17) ^ RightRotate32(v1, 19) ^ (v1 >> 10);
        v2 = w[i - 15];
        t2 = RightRotate32(v2, 7) ^ RightRotate32(v2, 18) ^ (v2 >> 3);
        w[i] = t1 + w[i - 7] + t2 + w[i - 16];
    }

    for (i = 0; i < 64; i++) {
        t1 = h + (RightRotate32(e, 6) ^ RightRotate32(e, 11) ^ RightRotate32(e, 25)) + ((e & f) ^ (~e & g)) + constants[i] + w[i];
        t2 = (RightRotate32(a, 2) ^ RightRotate32(a, 13) ^ RightRotate32(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state.hash[0] += a;
    state.hash[1] += b;
    state.hash[2] += c;
    state.hash[3] += d;
    state.hash[4] += e;
    state.hash[5] += f;
    state.hash[6] += g;
    state.hash[7] += h;
}
}
