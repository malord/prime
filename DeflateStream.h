// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DEFLATESTREAM_H
#define PRIME_DEFLATESTREAM_H

#include "Config.h"

#ifndef PRIME_NO_ZLIB

#include "ScopedPtr.h"
#include "Stream.h"
#include <zlib.h>

#define PRIME_HAVE_DEFLATESTREAM

namespace Prime {

/// A Stream implementation that compresses data using zlib.
class PRIME_PUBLIC DeflateStream : public Stream {
public:
    enum { defaultBufferSize = 32u * 1024u };

    DeflateStream();

    virtual ~DeflateStream();

    /// Initialise a deflation with the specified target. Returns false on error.
    /// If buffer is NULL, a buffer of the specified size is allocated.
    bool init(Stream* destination, Log* log, size_t bufferSize = defaultBufferSize, char* buffer = NULL);

    /// Ends the deflate. Call this once you know you're not going to write any more. Returns false if the
    /// deflation could not be completed.
    bool end(Log* log);

    /// Set compression level (0 through 9, default is 9).
    void setCompressionLevel(int zlibLevel)
    {
        PRIME_ASSERT(zlibLevel >= 0 && zlibLevel <= 9);
        _level = zlibLevel;
    }

    /// Returns the compression level that will be used by the next begin().
    int getCompressionLevel() const { return _level; }

    // Stream overrides.
    virtual ptrdiff_t writeSome(const void* bytes, size_t byteCount, Log* log) PRIME_OVERRIDE;
    virtual bool close(Log* log) PRIME_OVERRIDE;

private:
    void setBuffer(size_t size, char* buffer);

    /// Flush our internal buffer, and reset bufferPtr. If an error occurs, calls abort(). On error, returns false.
    bool flushBuffer(Log* log);

    /// Release the destination stream and reset begun.
    void cleanup();

    /// Free zlibs data and then calls cleanup().
    void abort();

    /// Log a zlib error.
    void logZlibError(Log* log, int err);

    ScopedArrayPtr<char> _deleteBuffer;
    char* _buffer;
    size_t _bufferSize;
    int _level;

    RefPtr<Stream> _dest;

    z_stream _zstream;
    bool _begun;

    char* _bufferPtr;
    char* _bufferTop;

    PRIME_UNCOPYABLE(DeflateStream);
};
}

#endif // PRIME_NO_ZLIB

#endif
