// Copyright 2000-2021 Mark H. P. Lord

#include "Substream.h"

namespace Prime {

PRIME_DEFINE_UID_CAST(Substream)

Substream::Substream()
{
    construct();
}

void Substream::construct()
{
    _discardWriteOverflow = false;
    _writeOverflowed = false;
}

Substream::Substream(Stream* stream, Offset baseOffset, bool seekToBaseOffset, Offset substreamSize, Log* log, bool seekable)
{
    construct();
    init(stream, baseOffset, seekToBaseOffset, substreamSize, log, seekable);
}

bool Substream::close(Log* log)
{
    if (!_stream) {
        return true;
    }

    bool success = _stream->close(log);
    _stream.release();

    return success;
}

bool Substream::init(Stream* stream, Offset baseOffset, bool seekToBaseOffset, Offset substreamSize, Log* log, bool seekable)
{
    PRIME_ASSERT(!seekable || stream->isSeekable());

    if (seekToBaseOffset) {
        PRIME_ASSERT(seekable);
        if (!stream->setOffset(baseOffset, log)) {
            return false;
        }
    }

    _stream = stream;
    _seekable = seekable;
    _base = baseOffset;
    _position = 0;
    _size = substreamSize;

    return true;
}

ptrdiff_t Substream::readSome(void* buffer, size_t maximumBytes, Log* log)
{
    PRIME_ASSERT(_stream);

    Offset remaining = _size - _position;

    if ((Offset)maximumBytes > remaining) {
        maximumBytes = (size_t)remaining;
    }

    if (!maximumBytes) {
        return 0;
    }

    ptrdiff_t got = _stream->readSome(buffer, maximumBytes, log);

    if (got < 0) {
        return got;
    }

    _position += got;

    return got;
}

ptrdiff_t Substream::writeSome(const void* memory, size_t maximumBytes, Log* log)
{
    PRIME_ASSERT(_stream);

    Offset remaining = _size - _position;

    size_t bytesToWrite = maximumBytes;

    if ((Offset)bytesToWrite > remaining) {
        _writeOverflowed = true;
        bytesToWrite = (size_t)remaining;
    }

    ptrdiff_t wrote = _stream->writeSome(memory, bytesToWrite, log);

    if (wrote < 0) {
        return wrote;
    }

    _position += wrote;

    if (_writeOverflowed && _discardWriteOverflow) {
        return maximumBytes;
    }

    return wrote;
}

Stream::Offset Substream::seek(Offset offset, SeekMode mode, Log* log)
{
    PRIME_ASSERT(_stream);

    if (!_seekable) {
        log->error(PRIME_LOCALISE("seek() called on non-seekable Substream."));
        return -1;
    }

    Offset newOffset = 0;

    switch (mode) {
    case SeekModeAbsolute:
        newOffset = offset;
        break;

    case SeekModeRelative:
        newOffset = _position + offset;
        break;

    case SeekModeRelativeToEnd:
        newOffset = _size + offset;
        break;

    default:
        PRIME_ASSERT(0);
        return -1;
    }

    if (newOffset < 0) {
        return -1;
    }

    if (newOffset > _size) {
        return -1;
    }

    _position = newOffset;

    Offset seekTo = _base + newOffset;

    if (!_stream->setOffset(seekTo, log)) {
        return -1;
    }

    return _position;
}

Stream::Offset Substream::getSize(Log*)
{
    PRIME_ASSERT(_stream);

    return _seekable ? _size : -1;
}

bool Substream::setSize(Offset, Log* log)
{
    log->error(PRIME_LOCALISE("Cannot set size of a Substream."));

    return false;
}

bool Substream::flush(Log* log)
{
    PRIME_ASSERT(_stream);

    return _stream->flush(log);
}

}
