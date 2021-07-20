// Copyright 2000-2021 Mark H. P. Lord

#include "Stream.h"
#include "NumberUtils.h"
#include "ScopedPtr.h"
#include "StringUtils.h"

namespace Prime {

PRIME_DEFINE_UID_CAST_BASE(Stream)

bool Stream::close(Log*)
{
    return true;
}

ptrdiff_t Stream::readSome(void*, size_t, Log* log)
{
    log->error(PRIME_LOCALISE("Stream not readable."));
    return -1;
}

ptrdiff_t Stream::writeSome(const void*, size_t, Log* log)
{
    log->error(PRIME_LOCALISE("Stream not writable."));
    return -1;
}

Stream::Offset Stream::seek(Offset, SeekMode, Log* log)
{
    log->error(PRIME_LOCALISE("Stream not seekable."));
    return -1;
}

Stream::Offset Stream::getSize(Log*)
{
    return -1;
}

bool Stream::setSize(Offset, Log* log)
{
    log->error(PRIME_LOCALISE("Stream size cannot be set."));
    return false;
}

bool Stream::flush(Log*)
{
    return true;
}

ptrdiff_t Stream::readAtOffset(Offset offset, void* buffer, size_t requiredBytes, Log* log)
{
    Offset oldOffset = getOffset(log);
    if (oldOffset < 0) {
        return -1;
    }

    if (!setOffset(offset, log)) {
        return -1;
    }

    ptrdiff_t got = read(buffer, requiredBytes, log);

    if (got < 0) {
        return -1;
    }

    if (!setOffset(oldOffset, log)) {
        return -1;
    }

    return got;
}

ptrdiff_t Stream::writeAtOffset(Offset offset, const void* bytes, size_t byteCount, Log* log)
{
    Offset oldOffset = getOffset(log);
    if (oldOffset < 0) {
        return -1;
    }

    if (!setOffset(offset, log)) {
        return -1;
    }

    ptrdiff_t wrote = write(bytes, byteCount, log);

    if (wrote < 0) {
        return -1;
    }

    if (!setOffset(oldOffset, log)) {
        return -1;
    }

    return wrote;
}

Stream* Stream::getUnderlyingStream() const
{
    return NULL;
}

bool Stream::copyFrom(Stream* source, Log* sourceLog, Offset length, Log* destLog, size_t bufferSize, void* buffer)
{
    bool error = false;
    if (source->tryCopyTo(error, this, destLog, length, sourceLog, bufferSize, buffer)) {
        return true;
    }
    if (error) {
        return false;
    }

    return copy(this, destLog, source, sourceLog, length, bufferSize, buffer);
}

bool Stream::tryCopyTo(bool& error, Stream* dest, Log* destLog, Offset length, Log* sourceLog,
    size_t bufferSize, void* buffer)
{
    error = false;
    (void)dest;
    (void)destLog;
    (void)length;
    (void)sourceLog;
    (void)bufferSize;
    (void)buffer;
    return false;
}

bool Stream::copy(Stream* dest, Log* destLog, Stream* source, Log* sourceLog, Offset length, size_t bufferSize,
    void* buffer)
{
    char stackBuffer[PRIME_BIG_STACK_BUFFER_SIZE];
    ScopedArrayPtr<char> heapBuffer;

    if (!buffer) {
        if (bufferSize <= sizeof(stackBuffer)) {
            buffer = stackBuffer;
            bufferSize = sizeof(stackBuffer);
        } else {
            heapBuffer.reset(new char[bufferSize]);
            buffer = heapBuffer.get();
        }
    }

    for (;;) {
        if (length == 0) {
            return true;
        }

        ptrdiff_t thisTime = (ptrdiff_t)(length < 0 ? (Offset)bufferSize : PRIME_MIN(length, (Offset)bufferSize));

        ptrdiff_t nread = source->readSome(buffer, thisTime, sourceLog);
        if (nread < 0) {
            return false;
        }

        if (nread == 0) {
            if (length < 0) {
                return true;
            }

            sourceLog->error(PRIME_LOCALISE("Unexpected end of file."));
            return false;
        }

        if (length >= 0) {
            length -= nread;
        }

        ptrdiff_t nwritten = dest->write(buffer, nread, destLog);
        if (nwritten < 0) {
            return false;
        }

        PRIME_ASSERT(nwritten == nread); // Being paranoid - write() should never allow this
    }
}

ptrdiff_t Stream::read(void* buffer, size_t maxBytes, Log* log)
{
    ptrdiff_t totalGot = 0;

    while (maxBytes) {
        ptrdiff_t got = readSome(buffer, maxBytes, log);
        if (got < 0) {
            return got;
        }

        if (got == 0) {
            break;
        }

        totalGot += got;
        maxBytes -= got;
        buffer = (char*)buffer + got;
    }

    return totalGot;
}

ptrdiff_t Stream::write(const void* bytes, size_t maxBytes, Log* log)
{
    ptrdiff_t totalWritten = 0;

    while (maxBytes) {
        ptrdiff_t wrote = writeSome(bytes, maxBytes, log);
        if (wrote < 0) {
            return wrote;
        }

        if (wrote == 0) {
            break;
        }

        totalWritten += wrote;
        maxBytes -= wrote;
        bytes = (const char*)bytes + wrote;
    }

    return totalWritten;
}

bool Stream::readExact(void* buffer, size_t byteCount, Log* log, const char* errorMessage)
{
    ptrdiff_t got = read(buffer, byteCount, log);
    if ((size_t)got == byteCount) {
        return true;
    }

    // If there was a read error, readSome() will have logged it.
    if (got < 0) {
        return false;
    }

    if (!errorMessage) {
        errorMessage = "Unexpected end of file";
    }

    log->error(PRIME_LOCALISE("%s (%" PRIdPTR "/%" PRIdPTR ")."), errorMessage, byteCount, got);
    return false;
}

bool Stream::readExact(Offset offset, void* buffer, size_t byteCount, Log* log, const char* errorMessage)
{
    if (!setOffset(offset, log)) {
        return false;
    }

    return readExact(buffer, byteCount, log, errorMessage);
}

bool Stream::writeExact(const void* bytes, size_t byteCount, Log* log, const char* errorMessage)
{
    ptrdiff_t wrote = write(bytes, byteCount, log);
    if ((size_t)wrote == byteCount) {
        return true;
    }

    // If there was a write error, writeSome() will have logged it.
    if (wrote < 0) {
        return false;
    }

    if (!errorMessage) {
        errorMessage = "Unable to write";
    }

    log->error(PRIME_LOCALISE("%s (%" PRIdPTR "/%" PRIdPTR " bytes)."), errorMessage, wrote, byteCount);
    return false;
}

bool Stream::writeExact(Offset offset, const void* bytes, size_t byteCount, Log* log, const char* errorMessage)
{
    if (!setOffset(offset, log)) {
        return false;
    }

    return writeExact(bytes, byteCount, log, errorMessage);
}

bool Stream::setOffset(Stream::Offset offset, Log* log)
{
    return seek(offset, SeekModeAbsolute, log) == offset;
}

bool Stream::skip(Offset distance, Log* log, const char* errorMessage)
{
    PRIME_ASSERT(distance >= 0);

    if (!distance) {
        return true;
    }

    char buffer[PRIME_BIG_STACK_BUFFER_SIZE];
    while (distance) {
        ptrdiff_t thisTime = (ptrdiff_t)Min<Offset>(sizeof(buffer), distance);

        ptrdiff_t got = read(buffer, thisTime, log);

        if (got < 0) {
            return false;
        }

        if (got != thisTime) {
            if (!errorMessage) {
                errorMessage = PRIME_LOCALISE("Unexpected end of file.");
            }

            log->error(errorMessage);
            return false;
        }

        distance -= thisTime;
    }

    return true;
}

bool Stream::printf(Log* log, const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    bool result = vprintf(log, format, argptr);
    va_end(argptr);
    return result;
}

bool Stream::vprintf(Log* log, const char* format, va_list argptr)
{
    FormatBufferVA<> formatted(format, argptr);

    return writeExact(formatted.c_str(), formatted.getLength(), log);
}

Stream* Stream::getMostUnderlyingStream() const
{
    Stream* search = const_cast<Stream*>(this);
    for (;;) {
        Stream* next = search->getUnderlyingStream();
        if (!next) {
            return search;
        }

        search = next;
    }
}

bool Stream::isSeekable()
{
    return getOffset(Log::getNullLog()) >= 0;
}

bool Stream::writeString(StringView string, Log* log)
{
    return write(string.data(), string.size(), log) >= 0;
}
}
