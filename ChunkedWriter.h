// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CHUNKEDWRITER_H
#define PRIME_CHUNKEDWRITER_H

#include "Config.h"
#include "Stream.h"

namespace Prime {

/// Implements HTTP chunked transfer coding writing.
class PRIME_PUBLIC ChunkedWriter : public Stream {
public:
    ChunkedWriter();

    explicit ChunkedWriter(Stream* stream);

    ~ChunkedWriter();

    void begin(Stream* stream);

    bool end(Log* log);

    /// Returns 0 at the end of the stream. It's up to the caller to read any trailing headers.
    virtual ptrdiff_t writeSome(const void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual bool close(Log* log) PRIME_OVERRIDE;

    Offset getBytesWritten() const { return _bytesWritten; }

private:
    void construct();

    RefPtr<Stream> _stream;
    bool _needEndWrite;
    Offset _bytesWritten;
};
}

#endif
