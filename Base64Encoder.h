// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_BASE64ENCODER_H
#define PRIME_BASE64ENCODER_H

#include "Config.h"
#include "ScopedPtr.h"
#include "Stream.h"
#include "StreamBuffer.h"

namespace Prime {

class PRIME_PUBLIC Base64Encoder : public Stream {
public:
    class PRIME_PUBLIC Options {
    public:
        explicit Options(size_t lineLength = 0)
            : _maxLineLength(lineLength)
        {
        }

        Options& setLineLength(size_t value)
        {
            PRIME_ASSERT(value >= 6);
            _maxLineLength = value;
            return *this;
        }
        size_t getLineLength() const { return _maxLineLength; }

    private:
        size_t _maxLineLength;
    };

    Base64Encoder();

    Base64Encoder(Stream* stream, const Options& options);

    ~Base64Encoder();

    void begin(Stream* stream, const Options& options);

    /// If an end-write isn't needed, does nothing and returns true.
    bool end(Log* log);

    virtual ptrdiff_t writeSome(const void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;

private:
    void construct();

    bool flushBuffer(Log* log, bool atEnd = false);

    bool _started;

    RefPtr<Stream> _stream;
    ScopedArrayPtr<uint8_t> _buffer;
    size_t _maxLineLength;
    size_t _bufferSize;
    size_t _bufferLength;
    Options _options;

    uint8_t _block[3];
    unsigned int _blockLength;
};
}

#endif
