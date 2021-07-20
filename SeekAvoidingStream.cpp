// Copyright 2000-2021 Mark H. P. Lord

#include "SeekAvoidingStream.h"

namespace Prime {

SeekAvoidingStream::SeekAvoidingStream()
    : _at(0)
{
}

SeekAvoidingStream::SeekAvoidingStream(Stream* underlyingStream, Offset at)
{
    init(underlyingStream, at);
}

SeekAvoidingStream::~SeekAvoidingStream()
{
}

void SeekAvoidingStream::setStream(Stream* underlyingStream)
{
    _underlyingStream = underlyingStream;
}

void SeekAvoidingStream::init(Stream* underlyingStream, Offset at)
{
    setStream(underlyingStream);

    setAt(at);
}

bool SeekAvoidingStream::close(Log* log)
{
    if (!_underlyingStream) {
        return true;
    }

    return _underlyingStream->close(log);
}

ptrdiff_t SeekAvoidingStream::readSome(void* buffer, size_t maxBytes, Log* log)
{
    ptrdiff_t got = _underlyingStream->readSome(buffer, maxBytes, log);
    if (got > 0) {
        _at += got;
    }
    return got;
}

ptrdiff_t SeekAvoidingStream::writeSome(const void* bytes, size_t maxBytes, Log* log)
{
    ptrdiff_t got = _underlyingStream->writeSome(bytes, maxBytes, log);
    if (got > 0) {
        _at += got;
    }
    return got;
}

ptrdiff_t SeekAvoidingStream::readAtOffset(Offset offset, void* buffer, size_t requiredBytes, Log* log)
{
    ptrdiff_t got = _underlyingStream->readAtOffset(offset, buffer, requiredBytes, log);
    if (got > 0) {
        _at = offset + got;
    }
    return got;
}

ptrdiff_t SeekAvoidingStream::writeAtOffset(Offset offset, const void* bytes, size_t byteCount, Log* log)
{
    ptrdiff_t got = _underlyingStream->writeAtOffset(offset, bytes, byteCount, log);
    if (got > 0) {
        _at = offset + got;
    }
    return got;
}

Stream::Offset SeekAvoidingStream::seek(Offset offset, SeekMode mode, Log* log)
{
    if (offset == _at) {
        return offset;
    }

    if (offset > _at) {
        if (!skip(offset - _at, log)) {
            return -1;
        }

        _at = offset;
        return offset;
    }

    log->trace("Seeking from: %" PRIME_PRId_STREAM " to %" PRIME_PRId_STREAM ".");

    Offset newOffset = _underlyingStream->seek(offset, mode, log);
    if (newOffset >= 0) {
        _at = newOffset;
    }
    return newOffset;
}

Stream::Offset SeekAvoidingStream::getSize(Log* log)
{
    return _underlyingStream->getSize(log);
}

bool SeekAvoidingStream::setSize(Offset newSize, Log* log)
{
    return _underlyingStream->setSize(newSize, log);
}

bool SeekAvoidingStream::flush(Log* log)
{
    return _underlyingStream->flush(log);
}

bool SeekAvoidingStream::isSeekable()
{
    return _underlyingStream->isSeekable();
}

Stream* SeekAvoidingStream::getUnderlyingStream() const
{
    return _underlyingStream;
}
}
