// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_INFLATESTREAM_H
#define PRIME_INFLATESTREAM_H

#include "Config.h"

#ifndef PRIME_NO_ZLIB

#include "ScopedPtr.h"
#include "Stream.h"
#include <zlib.h>

#define PRIME_HAVE_INFLATESTREAM

namespace Prime {

/// A Stream implementation that decompresses data using zlib.
class PRIME_PUBLIC InflateStream : public Stream {
public:
    enum { defaultBufferSize = 8192u };

    InflateStream();

    virtual ~InflateStream();

    /// Initialise an inflation with the specified source.
    /// If buffer is NULL, a buffer of the specified size is allocated.
    bool init(Stream* sourceStream, Log* log, size_t bufferSize = defaultBufferSize, char* buffer = NULL);

    /// Ends the inflate.
    void end();

    /// If you know the size of the decompressed data, call this to set it otherwise getSize() will return -1.
    void setSizeKnown(int64_t size) { _sizeKnown = size; }

    /// If you don't know the size, call this. This is the default.
    void setSizeNotKnown() { _sizeKnown = -1; }

    // Stream overrides.
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readSome(void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual int64_t getSize(Log* log) PRIME_OVERRIDE;

private:
    void setBuffer(size_t bufferSize, char* buffer);

    /// Release the destination stream and reset begun.
    void cleanup();

    /// Log a zlib error.
    void logZlibError(Log* log, int err);

    ScopedArrayPtr<char> _deleteBuffer;
    char* _buffer;
    size_t _bufferSize;

    RefPtr<Stream> _source;

    z_stream _zstream;
    bool _eof;
    bool _begun;

    char* _bufferPtr;
    char* _bufferTop;

    int64_t _sizeKnown;

    PRIME_UNCOPYABLE(InflateStream);
};
}

#endif // PRIME_NO_ZLIB

#endif
