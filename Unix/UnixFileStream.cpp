// Copyright 2000-2021 Mark H. P. Lord

#include "UnixFileStream.h"
#include "UnixCloseOnExec.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Prime {

PRIME_DEFINE_UID_CAST(UnixFileStream)

UnixFileStream::UnixFileStream()
{
    _handle = -1;
    _syncOnClose = false;
    _shouldClose = false;
}

UnixFileStream::~UnixFileStream()
{
    close(Log::getNullLog());
}

bool UnixFileStream::open(const char* path, const OpenMode& openMode, Log* log)
{
    int unixFlags = 0;

    if (openMode.getReadWrite()) {
        unixFlags |= O_RDWR;
    } else if (openMode.getRead()) {
        unixFlags |= O_RDONLY;
    } else if (openMode.getWrite()) {
        unixFlags |= O_WRONLY;
    }

    if (openMode.getCreate()) {
        unixFlags |= O_CREAT;
    }

    if (openMode.getDoNotOverwrite()) {
        unixFlags |= O_EXCL;
    }

    if (openMode.getTruncate()) {
        unixFlags |= O_TRUNC;
    }

    if (openMode.getAppend()) {
        unixFlags |= O_APPEND;
    }

    return unixOpen(path, unixFlags, log, openMode);
}

bool UnixFileStream::unixOpen(const char* path, int unixFlags, Log* log, const OpenMode& openMode)
{
    close(log);

    int permissions;

    if (openMode.getUseUnixPermissions()) {
        permissions = openMode.getUnixPermissions();
    } else {
        permissions = 0666;
    }

// O_CLOEXEC has been known to be defined but then not actually do anything, so I use it but still use
// the old fashioned mutex-and-fcntl mechanism.
#ifdef O_CLOEXEC
    if (!openMode.getChildProcessInherit()) {
        unixFlags |= O_CLOEXEC;
    }
#endif

    if (!(unixFlags & O_CREAT)) {
        permissions = 0;
    }

    UnixCloseOnExec::ScopedLock execLock;

    int openedHandle;
    for (;;) {
        openedHandle = ::open(path, unixFlags, permissions);
        if (openedHandle >= 0) {
            break;
        }

        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }

    if (!openMode.getChildProcessInherit()) {
        UnixCloseOnExec::closeOnExec(openedHandle);
    }

    attach(openedHandle, true, openMode.getSyncOnClose());

    return true;
}

void UnixFileStream::attach(int existingHandle, bool closeWhenDone, bool syncOnClose)
{
    close(Log::getNullLog());

    _handle = existingHandle;
    _shouldClose = closeWhenDone;
    _syncOnClose = syncOnClose;
}

int UnixFileStream::detach()
{
    int detached = _handle;

    _handle = -1;
    _shouldClose = false;
    _syncOnClose = false;

    return detached;
}

bool UnixFileStream::close(Log* log)
{
    bool result = true;

    if (_shouldClose) {
        if (_syncOnClose) {
            while (::fsync(_handle) < 0) {
                if (errno != EINTR) {
                    log->logErrno(errno);
                    result = false;
                    break;
                }
            }
        }

        while (::close(_handle) < 0) {
            if (errno != EINTR) {
                log->logErrno(errno);
                result = false;
                break;
            }
        }

        _shouldClose = false;
    }

    _handle = -1;
    return result;
}

ptrdiff_t UnixFileStream::readSome(void* buffer, size_t maxBytes, Log* log)
{
    PRIME_ASSERT(isOpen());

    ptrdiff_t bytesRead;
    for (;;) {
        bytesRead = ::read(_handle, buffer, maxBytes);
        if (bytesRead >= 0) {
            break;
        }

        if (errno != EINTR) {
            log->logErrno(errno);
            break;
        }
    }

    return bytesRead;
}

ptrdiff_t UnixFileStream::writeSome(const void* bytes, size_t maxBytes, Log* log)
{
    PRIME_ASSERT(isOpen());

    ptrdiff_t bytesWritten;
    for (;;) {
        bytesWritten = ::write(_handle, bytes, maxBytes);
        if (bytesWritten >= 0) {
            break;
        }

        if (errno != EINTR) {
            log->logErrno(errno);
            break;
        }
    }

    return bytesWritten;
}

Stream::Offset UnixFileStream::seek(Offset offset, SeekMode mode, Log* log)
{
    PRIME_ASSERT(isOpen());

    int unixMode;
    switch (mode) {
    case SeekModeAbsolute:
        unixMode = SEEK_SET;
        break;
    case SeekModeRelative:
        unixMode = SEEK_CUR;
        break;
    case SeekModeRelativeToEnd:
        unixMode = SEEK_END;
        break;
    default:
        PRIME_ASSERT(0);
        return -1;
    }

    Offset result;
    for (;;) {
        result = lseek(_handle, offset, unixMode);
        if (result >= 0) {
            break;
        }

        if (errno != EINTR) {
            log->logErrno(errno);
            break;
        }
    }

    return result;
}

Stream::Offset UnixFileStream::getSize(Log* log)
{
    PRIME_ASSERT(isOpen());

    struct ::stat ss;
    for (;;) {
        memset(&ss, 0, sizeof(ss));
        if (::fstat(_handle, &ss) == 0) {
            break;
        }

        if (errno != EINTR) {
            log->logErrno(errno);
            return -1;
        }
    }

    return ss.st_size;
}

bool UnixFileStream::setSize(Offset newSize, Log* log)
{
    PRIME_ASSERT(isOpen());

    for (;;) {
        if (::ftruncate(_handle, newSize) == 0) {
            break;
        }

        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }

    return true;
}

bool UnixFileStream::flush(Log*)
{
    return true;
}
}
