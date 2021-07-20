// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_BASE64DECODER_H
#define PRIME_BASE64DECODER_H

#include "Config.h"
#include "Stream.h"
#include "StreamBuffer.h"

namespace Prime {

class PRIME_PUBLIC Base64Decoder : public Stream {
public:
    Base64Decoder();

    explicit Base64Decoder(StreamBuffer* buffer);

    ~Base64Decoder();

    void begin(StreamBuffer* buffer);

    virtual ptrdiff_t readSome(void* buffer, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;

private:
    void construct();

    bool _started;

    RefPtr<StreamBuffer> _buffer;

    struct State {
        uint8_t block[4];
        unsigned int blockLength;
        unsigned int padCount;
        uint8_t decoded[3];
        unsigned int decodedLength;
    } _state;
};
}

#endif
