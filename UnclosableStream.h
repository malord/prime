// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNCLOSABLESTREAM_H
#define PRIME_UNCLOSABLESTREAM_H

#include "Stream.h"

namespace Prime {

/// Wrap another Stream so that a call to close() will not close the underlying Stream.
class PRIME_PUBLIC UnclosableStream : public Stream {
public:
    UnclosableStream();

    explicit UnclosableStream(Stream* underlyingStream);

    ~UnclosableStream();

    void setStream(Stream* underlyingStream);

    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readSome(void* buffer, size_t maxBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeSome(const void* bytes, size_t maxBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readAtOffset(Offset offset, void* buffer, size_t requiredBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeAtOffset(Offset offset, const void* bytes, size_t byteCount, Log* log) PRIME_OVERRIDE;
    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE;
    virtual Offset getSize(Log* log) PRIME_OVERRIDE;
    virtual bool setSize(Offset newSize, Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;
    virtual bool copyFrom(Stream* source, Log* sourceLog, Offset length, Log* destLog, size_t bufferSize = 0,
        void* buffer = NULL) PRIME_OVERRIDE;
    virtual Stream* getUnderlyingStream() const PRIME_OVERRIDE;

private:
    RefPtr<Stream> _underlyingStream;
};
}

#endif
