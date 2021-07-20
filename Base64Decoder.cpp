// Copyright 2000-2021 Mark H. P. Lord

#include "Base64Decoder.h"
#include "TextEncoding.h"

namespace Prime {

Base64Decoder::Base64Decoder()
{
    construct();
}

Base64Decoder::Base64Decoder(StreamBuffer* buffer)
{
    construct();
    begin(buffer);
}

Base64Decoder::~Base64Decoder()
{
}

void Base64Decoder::construct()
{
    Base64::BuildDecodingTable();
    _started = false;
}

void Base64Decoder::begin(StreamBuffer* buffer)
{
    _buffer = buffer;

    _state.blockLength = 0;
    _state.decodedLength = 0;
    _state.padCount = 0;

    _started = true;
}

ptrdiff_t Base64Decoder::readSome(void* memory, size_t maximumBytes, Log* log)
{
    PRIME_ASSERT(_started);

    uint8_t* dest = (uint8_t*)memory;
    uint8_t* destEnd = dest + maximumBytes;

    const uint8_t* leftover = _state.decoded;
    const uint8_t* leftoverEnd = leftover + _state.decodedLength;

    while (leftover != leftoverEnd) {
        if (dest == destEnd) {
            _state.decodedLength = (unsigned int)(leftoverEnd - leftover);
            memmove(&_state.decoded[0], leftover, _state.decodedLength);
            return dest - (const uint8_t*)memory;
        }

        *dest++ = *leftover++;
    }

    _state.decodedLength = 0;

    State state = _state;

    for (;;) {
        const uint8_t* ptr = (const uint8_t*)_buffer->getReadPointer();
        const uint8_t* end = (const uint8_t*)_buffer->getTopPointer();

        if (ptr == end) {
            ptrdiff_t got = _buffer->fetchMore(log);
            if (got < 0) {
                return -1;
            }
            if (got == 0) {
                // Discard any incomplete block.
                _state = state;
                if (_state.blockLength) {
                    log->warning(PRIME_LOCALISE("Incomplete Base-64 block"));
                }
                return dest - (const uint8_t*)memory;
            }

            ptr = (const uint8_t*)_buffer->getReadPointer();
            end = (const uint8_t*)_buffer->getTopPointer();
        }

        while (ptr != end) {
            uint8_t decoded = Base64::decodingTable[*ptr++];
            if (decoded == Base64::decodingTableInvalidChar) {
                // Ignore characters we don't recognise.
                continue;
            }

            if (decoded == Base64::decodingTablePadChar) {
                ++state.padCount;
                state.block[state.blockLength++] = 0;

            } else {
                state.block[state.blockLength++] = decoded;
            }

            if (state.blockLength < 4) {
                continue;
            }

            PRIME_DEBUG_ASSERT(state.blockLength == 4);
            state.blockLength = 0;

            if (state.padCount > 2) {
                // Invalid block.
                log->warning(PRIME_LOCALISE("Invalid Base-64 block."));
                _state.blockLength = 0;
                _state.padCount = 0;
                continue;
            }

            unsigned int decodedLength = 3 - state.padCount;
            state.padCount = 0;

            if (destEnd - dest >= 3) {
                if (decodedLength == 3) {
                    Base64::DecodeBlock(dest, state.block);
                    dest += 3;

                } else {
                    Base64::DecodeBlock(state.decoded, state.block);
                    memcpy(dest, state.decoded, decodedLength);
                    dest += decodedLength;
                }

            } else {
                Base64::DecodeBlock(state.decoded, state.block);
                unsigned int destRemaining = (unsigned int)(destEnd - dest);
                memcpy(dest, state.decoded, destRemaining);
                dest += destRemaining;
                memmove(&state.decoded[0], &state.decoded[destRemaining], decodedLength - destRemaining);
                state.decodedLength = decodedLength - (unsigned int)destRemaining;
                _state = state;
                _buffer->setReadPointer(ptr);
                return dest - (const uint8_t*)memory;
            }
        }

        _buffer->setReadPointer(ptr);
    }
}

bool Base64Decoder::close(Log* log)
{
    if (_started) {
        _started = false;
        return _buffer->close(log);
    }

    return true;
}

bool Base64Decoder::flush(Log* log)
{
    if (_started) {
        return _buffer->flush(log);
    }

    return true;
}

}
