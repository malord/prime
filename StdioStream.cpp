// Copyright 2000-2021 Mark H. P. Lord

#include "StdioStream.h"
#include "StdioUtils.h"
#include <errno.h>

namespace Prime {

PRIME_DEFINE_UID_CAST(StdioStream)

StdioStream::StdioStream()
{
    construct();
}

StdioStream::StdioStream(FILE* existingFile, bool closeWhenDone)
{
    construct();
    attach(existingFile, closeWhenDone);
}

StdioStream::StdioStream(const char* filename, const char* fopenMode, Log* log, const OpenMode& openMode)
{
    construct();
    fopen(filename, fopenMode, log, openMode);
}

StdioStream::~StdioStream()
{
    close(Log::getNullLog());
}

bool StdioStream::attach(FILE* existingFile, bool closeWhenDone)
{
    bool success = close(Log::getNullLog());

    _fp = existingFile;
    _shouldClose = closeWhenDone;

    return success;
}

FILE* StdioStream::detach()
{
    FILE* detached = _fp;

    _fp = NULL;
    _shouldClose = false;

    return detached;
}

bool StdioStream::fopen(const char* filename, const char* fopenMode, Log* log, const OpenMode& openMode)
{
    close(Log::getNullLog());

    const bool forceNoInherit = !openMode.getChildProcessInherit();

    FILE* openedFile = StdioOpen(filename, fopenMode, forceNoInherit);

    if (!openedFile) {
        log->logErrno(errno);
        return false;
    }

    attach(openedFile, true);
    return true;
}

bool StdioStream::open(const char* filename, const OpenMode& openMode, Log* log)
{
    if (openMode.getRead() && !openMode.getWrite() && !openMode.getCreate() && !openMode.getTruncate() && !openMode.getAppend()) {
        return fopen(filename, "rb", log, openMode);
    }

    if (!openMode.getRead() && openMode.getWrite() && openMode.getCreate() && openMode.getTruncate() && !openMode.getAppend()) {
        return fopen(filename, "wb", log, openMode);
    }

    if (openMode.getRead() && openMode.getWrite() && openMode.getCreate() && !openMode.getTruncate() && !openMode.getAppend()) {
        return fopen(filename, "r+b", log, openMode);
    }

    if (openMode.getRead() && openMode.getWrite() && openMode.getCreate() && openMode.getTruncate() && !openMode.getAppend()) {
        return fopen(filename, "w+b", log, openMode);
    }

    if (!openMode.getRead() && openMode.getWrite() && openMode.getCreate() && !openMode.getTruncate() && openMode.getAppend()) {
        return fopen(filename, "ab", log, openMode);
    }

    if (openMode.getRead() && openMode.getWrite() && openMode.getCreate() && !openMode.getTruncate() && openMode.getAppend()) {
        return fopen(filename, "a+b", log, openMode);
    }

    log->error(PRIME_LOCALISE("StdioStream: unsupported open flags."));
    return false;
}

void StdioStream::setBinaryMode()
{
    PRIME_ASSERT(isOpen());

    StdioSetModeBinary(_fp);
}

void StdioStream::setTextMode()
{
    PRIME_ASSERT(isOpen());

    StdioSetModeText(_fp);
}

bool StdioStream::close(Log* log)
{
    bool success = true;

    if (_shouldClose) {
        PRIME_ASSERT(_fp);
        if (fclose(_fp) != 0) {
            log->logErrno(errno);
            success = false;
        }
    }

    zero();

    return success;
}

ptrdiff_t StdioStream::readSome(void* buffer, size_t maxBytes, Log* log)
{
    PRIME_ASSERT(isOpen());

    ptrdiff_t result = StdioRead(_fp, buffer, maxBytes);

    if (result < 0) {
        log->logErrno(errno);
    }

    return result;
}

ptrdiff_t StdioStream::writeSome(const void* bytes, size_t maxBytes, Log* log)
{
    PRIME_ASSERT(isOpen());

    ptrdiff_t result = StdioWrite(_fp, bytes, maxBytes);

    if (result < 0) {
        log->logErrno(errno);
    }

    return result;
}

Stream::Offset StdioStream::seek(Offset offset, SeekMode mode, Log* log)
{
    PRIME_ASSERT(isOpen());

    if (mode != SeekModeRelative || offset != 0) {
        int result;

        switch (mode) {
        case SeekModeAbsolute:
        default:
            result = StdioSeek(_fp, offset, SEEK_SET);
            break;

        case SeekModeRelative:
            result = StdioSeek(_fp, offset, SEEK_CUR);
            break;

        case SeekModeRelativeToEnd:
            result = StdioSeek(_fp, offset, SEEK_END);
            break;
        }

        if (result != 0) {
            log->logErrno(errno);
            return -1;
        }
    }

    int64_t where = StdioTell(_fp);
    if (where >= 0) {
        return where;
    }

    log->logErrno(errno);
    return -1;
}

Stream::Offset StdioStream::getSize(Log* log)
{
    int64_t pos = getOffset(log);
    if (pos < 0) {
        return -1;
    }

    if (seek(0, SeekModeRelativeToEnd, log) < 0) {
        return -1;
    }

    int64_t size = getOffset(log);
    if (size < 0) {
        return -1;
    }

    if (seek(pos, SeekModeAbsolute, log) < 0) {
        return -1;
    }

    return size;
}

bool StdioStream::setSize(Offset, Log*)
{
    return false;
}

bool StdioStream::flush(Log* log)
{
    if (!isOpen()) {
        return false;
    }

    if (fflush(_fp) == 0) {
        return true;
    }

    log->logErrno(errno);
    return false;
}
}
