// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CHUNKEDREADER_H
#define PRIME_CHUNKEDREADER_H

#include "Config.h"
#include "StreamBuffer.h"

namespace Prime {

/// Read HTTP chunked transfer coding.
class PRIME_PUBLIC ChunkedReader : public Stream {
    PRIME_DECLARE_UID_CAST(Stream, 0x4467a3ad, 0xdf5640a2, 0x968bccc1, 0xac5cac82)

public:
    ChunkedReader();

    explicit ChunkedReader(StreamBuffer* buffer);

    ~ChunkedReader();

    void begin(StreamBuffer* buffer);

    /// Returns true if we've reached the end of the chunked data. This will read the next chunk header if
    /// necessary, and can therefore fail.
    bool hasFinished(Log* log);

    /// Returns 0 at the end of the stream. It's up to the caller to read any trailing headers from the
    /// underlying Stream.
    virtual ptrdiff_t readSome(void* buffer, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual bool close(Log* log) PRIME_OVERRIDE;

private:
    void construct();

    bool readChunkSize(Log* log);

    RefPtr<StreamBuffer> _buffer;
    int64_t _chunkRemaining;
    bool _finished;
};
}

#endif
