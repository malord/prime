// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SUBSTREAM_H
#define PRIME_SUBSTREAM_H

#include "Stream.h"

namespace Prime {

/// A Stream that restricts access to a region ("sub-stream") of another stream.
class PRIME_PUBLIC Substream : public Stream {
    PRIME_DECLARE_UID_CAST(Stream, 0x89b34dc4, 0x5f664162, 0xa647b99e, 0xe434345c)

public:
    Substream();

    /// See init().
    Substream(Stream* stream, Offset baseOffset, bool seekToBaseOffset, Offset substreamSize, Log* log,
        bool seekable = true);

    /// Set the Stream and region to access. Returns false if the stream is not seekable. If seekToBaseOffset is
    /// false then the stream must already be at that offset.
    bool init(Stream* stream, Offset baseOffset, bool seekToBaseOffset, Offset substreamSize, Log* log,
        bool seekable = true);

    /// If enabled, writeSome() will fein success if too many bytes are written. Use didWriteOverflow() to find
    /// out if this occurred. This is disabled by default (writeSome() will indicate disk-full to the caller if
    /// it attempts to write beyond the substream region).
    void setSilentlyDetectWriteOverflow(bool enable) { _discardWriteOverflow = enable; }

    bool didWriteOverflow() const { return _writeOverflowed; }

    void setWriteOverflowed(bool value) { _writeOverflowed = value; }

    Offset getBaseOffset() const { return _base; }

    Offset getOffset() const { return _position; }

    Offset getUnderlyingStreamOffset() const { return _base + _position; }

    // Stream overrides.
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readSome(void* buffer, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeSome(const void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE;
    virtual Offset getSize(Log* log) PRIME_OVERRIDE;
    virtual bool setSize(Offset newSize, Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;
    virtual Stream* getUnderlyingStream() const PRIME_OVERRIDE { return _stream; }

private:
    void construct();

    RefPtr<Stream> _stream;
    Offset _base;
    Offset _position;
    Offset _size;
    bool _seekable;

    bool _discardWriteOverflow;
    bool _writeOverflowed;

    PRIME_UNCOPYABLE(Substream);
};

}

#endif
