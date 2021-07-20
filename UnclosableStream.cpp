// Copyright 2000-2021 Mark H. P. Lord

#include "UnclosableStream.h"

namespace Prime {

UnclosableStream::UnclosableStream()
{
}

UnclosableStream::UnclosableStream(Stream* underlyingStream)
    : _underlyingStream(underlyingStream)
{
}

UnclosableStream::~UnclosableStream()
{
}

void UnclosableStream::setStream(Stream* underlyingStream)
{
    _underlyingStream = underlyingStream;
}

bool UnclosableStream::close(Log*)
{
    return true;
}

ptrdiff_t UnclosableStream::readSome(void* buffer, size_t maxBytes, Log* log)
{
    return _underlyingStream->readSome(buffer, maxBytes, log);
}

ptrdiff_t UnclosableStream::writeSome(const void* bytes, size_t maxBytes, Log* log)
{
    return _underlyingStream->writeSome(bytes, maxBytes, log);
}

ptrdiff_t UnclosableStream::readAtOffset(Offset offset, void* buffer, size_t requiredBytes, Log* log)
{
    return _underlyingStream->readAtOffset(offset, buffer, requiredBytes, log);
}

ptrdiff_t UnclosableStream::writeAtOffset(Offset offset, const void* bytes, size_t byteCount, Log* log)
{
    return _underlyingStream->writeAtOffset(offset, bytes, byteCount, log);
}

Stream::Offset UnclosableStream::seek(Offset offset, SeekMode mode, Log* log)
{
    return _underlyingStream->seek(offset, mode, log);
}

Stream::Offset UnclosableStream::getSize(Log* log)
{
    return _underlyingStream->getSize(log);
}

bool UnclosableStream::setSize(Offset newSize, Log* log)
{
    return _underlyingStream->setSize(newSize, log);
}

bool UnclosableStream::flush(Log* log)
{
    return _underlyingStream->flush(log);
}

bool UnclosableStream::copyFrom(Stream* source, Log* sourceLog, Offset length, Log* destLog, size_t bufferSize, void* buffer)
{
    return _underlyingStream->copyFrom(source, sourceLog, length, destLog, bufferSize, buffer);
}

Stream* UnclosableStream::getUnderlyingStream() const
{
    return _underlyingStream;
}

}
