// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_GZIPWRITER_H
#define PRIME_GZIPWRITER_H

#include "DeflateStream.h"

#ifdef PRIME_HAVE_DEFLATESTREAM

#include "CRC32.h"
#include "HashStream.h"

#define PRIME_HAVE_GZIPWRITER

namespace Prime {

/// Writes a gzip header then compresses anything written to the Stream, and appends a gzip footer at the end.
class PRIME_PUBLIC GZipWriter : public Stream {
public:
    GZipWriter();

    ~GZipWriter();

    bool begin(Stream* underlyingStream, int compressionLevel, Log* log);

    Offset getBytesWritten() const { return _bytesWritten; }

    bool end(Log* log);

    virtual ptrdiff_t writeSome(const void* bytes, size_t maxBytes, Log* log) PRIME_OVERRIDE;
    virtual bool close(Log* log) PRIME_OVERRIDE;

private:
    RefPtr<Stream> _underlyingStream;
    DeflateStream _deflater;
    HashStream<CRC32> _crcer;
    Offset _bytesWritten;
    bool _begun;
};
}

#endif // PRIME_HAVE_DEFLATESTREAM

#endif
