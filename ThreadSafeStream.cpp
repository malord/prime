// Copyright 2000-2021 Mark H. P. Lord

#include "ThreadSafeStream.h"

namespace Prime {

ThreadSafeStream::ThreadSafeStream()
{
}

ThreadSafeStream::~ThreadSafeStream()
{
}

bool ThreadSafeStream::init(Stream* wrap, Log* log)
{
    if (!_mutex.init(log)) {
        return false;
    }

    _underlyingStream = wrap;

    return true;
}

Stream* ThreadSafeStream::lockStream()
{
    _mutex.lock();
    return _underlyingStream;
}

void ThreadSafeStream::unlockStream()
{
    _mutex.unlock();
}

ptrdiff_t ThreadSafeStream::readSome(void* buffer, size_t maximumBytes, Log* log)
{
    return ScopedLock(this)->readSome(buffer, maximumBytes, log);
}

ptrdiff_t ThreadSafeStream::writeSome(const void* memory, size_t maximumBytes, Log* log)
{
    return ScopedLock(this)->writeSome(memory, maximumBytes, log);
}

Stream::Offset ThreadSafeStream::seek(Offset offset, SeekMode mode, Log* log)
{
    return ScopedLock(this)->seek(offset, mode, log);
}

Stream::Offset ThreadSafeStream::getSize(Log* log)
{
    return ScopedLock(this)->getSize(log);
}

bool ThreadSafeStream::setSize(Offset size, Log* log)
{
    return ScopedLock(this)->setSize(size, log);
}

ptrdiff_t ThreadSafeStream::readAtOffset(Offset offset, void* buffer, size_t requiredBytes, Log* log)
{
    return ScopedLock(this)->readAtOffset(offset, buffer, requiredBytes, log);
}

ptrdiff_t ThreadSafeStream::writeAtOffset(Offset offset, const void* bytes, size_t byteCount, Log* log)
{
    return ScopedLock(this)->writeAtOffset(offset, bytes, byteCount, log);
}

bool ThreadSafeStream::close(Log* log)
{
    RecursiveMutex::ScopedLock lock(&_mutex);

    bool ok = true;

    if (_underlyingStream) {
        ok = _underlyingStream->close(log);
        _underlyingStream = NULL;
    }

    return ok;
}

bool ThreadSafeStream::flush(Log* log)
{
    return ScopedLock(this)->flush(log);
}

bool ThreadSafeStream::copyFrom(Stream* source, Log* sourceLog, Offset length, Log* destLog, size_t bufferSize, void* buffer)
{
    return ScopedLock(this)->copyFrom(source, sourceLog, length, destLog, bufferSize, buffer);
}
}
