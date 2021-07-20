// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_QUOTEDPRINTABLEDECODER_H
#define PRIME_QUOTEDPRINTABLEDECODER_H

#include "Config.h"
#include "ScopedPtr.h"
#include "Stream.h"
#include "StreamBuffer.h"

namespace Prime {

/// Decode quoted-printable encoding from an underlying Stream.
class PRIME_PUBLIC QuotedPrintableDecoder : public Stream {
public:
    QuotedPrintableDecoder();

    explicit QuotedPrintableDecoder(StreamBuffer* buffer);

    ~QuotedPrintableDecoder();

    void begin(StreamBuffer* buffer);

    virtual ptrdiff_t readSome(void* buffer, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;

private:
    void construct();

    bool _started;

    RefPtr<StreamBuffer> _buffer;
};
}

#endif
