// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SEEKAVOIDINGSTREAM_H
#define PRIME_SEEKAVOIDINGSTREAM_H

#include "Stream.h"

namespace Prime {

/// A Stream which keeps track of the file pointer offset of an underlying Stream and avoid calls to the
/// underlying Stream's seek() method if possible. Seeking forward in the Stream is implemented by skipping
/// (reading and discarding bytes).
class PRIME_PUBLIC SeekAvoidingStream : public Stream {
public:
    SeekAvoidingStream();

    /// Calls init()
    explicit SeekAvoidingStream(Stream* underlyingStream, Offset at = 0);

    ~SeekAvoidingStream();

    void init(Stream* underlyingStream, Offset at = 0);

    void setStream(Stream* underlyingStream);

    Offset getAt() const PRIME_NOEXCEPT { return _at; }

    void setAt(Offset at) PRIME_NOEXCEPT { _at = at; }

    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readSome(void* buffer, size_t maxBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeSome(const void* bytes, size_t maxBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readAtOffset(Offset offset, void* buffer, size_t requiredBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeAtOffset(Offset offset, const void* bytes, size_t byteCount, Log* log) PRIME_OVERRIDE;
    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE;
    virtual Offset getSize(Log* log) PRIME_OVERRIDE;
    virtual bool setSize(Offset newSize, Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;
    virtual bool isSeekable() PRIME_OVERRIDE;
    virtual Stream* getUnderlyingStream() const PRIME_OVERRIDE;

private:
    RefPtr<Stream> _underlyingStream;
    Offset _at;
};
}

#endif
