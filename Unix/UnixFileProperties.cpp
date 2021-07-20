// Copyright 2000-2021 Mark H. P. Lord

#include "UnixFileProperties.h"
#include "../Path.h"
#include "../Templates.h"
#include <errno.h>
#include <sys/time.h>

#ifdef PRIME_OS_BSD
#define st_atim st_atimespec
#define st_ctim st_ctimespec
#define st_mtim st_mtimespec
#endif

namespace Prime {

bool UnixFileProperties::read(const char* path, Log* log)
{
    _hidden = StringStartsWith(Path::lastComponentView(path), '.');

    for (;;) {
        memset(&_stat, 0, sizeof(_stat));
        _initialised = (::stat(path, &_stat) == 0);
        if (_initialised) {
            return true;
        }

        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }
}

bool UnixFileProperties::readLink(const char* path, Log* log)
{
    _hidden = StringStartsWith(Path::lastComponentView(path), '.');

    for (;;) {
        memset(&_stat, 0, sizeof(_stat));
        _initialised = (::lstat(path, &_stat) == 0);
        if (_initialised) {
            return true;
        }

        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }
}

bool UnixFileProperties::readHandle(int handle, Log* log)
{
    for (;;) {
        memset(&_stat, 0, sizeof(_stat));
        _initialised = (::fstat(handle, &_stat) == 0);
        if (_initialised) {
            return true;
        }

        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }
}

bool UnixFileProperties::applyTimes(const char* path, Log* log) const
{
    struct timeval times[2];
#ifdef PRIME_OS_QNX
    times[0].tv_sec = _stat.st_atime;
    times[0].tv_usec = 0;
    times[1].tv_sec = _stat.st_mtime;
    times[1].tv_usec = 0;
#elif defined(PRIME_OS_ANDROID)
    times[0].tv_sec = _stat.st_atime;
    times[0].tv_usec = (int32_t)(_stat.st_atime_nsec / 1000);
    times[1].tv_sec = _stat.st_mtime;
    times[1].tv_usec = (int32_t)(_stat.st_mtime_nsec / 1000);
#else
    times[0].tv_sec = _stat.st_atim.tv_sec;
    times[0].tv_usec = (int32_t)(_stat.st_atim.tv_nsec / 1000);
    times[1].tv_sec = _stat.st_mtim.tv_sec;
    times[1].tv_usec = (int32_t)(_stat.st_mtim.tv_nsec / 1000);
#endif

    while (utimes(path, times) != 0) {
        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }

    return true;
}

UnixTime UnixFileProperties::getStatusChangeTime() const
{
    PRIME_ASSERT(_initialised);
#ifdef PRIME_OS_QNX
    return UnixTime((int64_t)_stat.st_ctime, 0);
#elif defined(PRIME_OS_ANDROID)
    return UnixTime((int64_t)_stat.st_ctime, (int32_t)_stat.st_ctime_nsec);
#else
    return UnixTime((int64_t)_stat.st_ctim.tv_sec, (int32_t)_stat.st_ctim.tv_nsec);
#endif
}

UnixTime UnixFileProperties::getLastAccessTime() const
{
    PRIME_ASSERT(_initialised);
#ifdef PRIME_OS_QNX
    return UnixTime((int64_t)_stat.st_atime, 0);
#elif defined(PRIME_OS_ANDROID)
    return UnixTime((int64_t)_stat.st_atime, (int32_t)_stat.st_atime_nsec);
#else
    return UnixTime((int64_t)_stat.st_atim.tv_sec, (int32_t)_stat.st_atim.tv_nsec);
#endif
}

UnixTime UnixFileProperties::getModificationTime() const
{
    PRIME_ASSERT(_initialised);
#ifdef PRIME_OS_QNX
    return UnixTime((int64_t)_stat.st_mtime, 0);
#elif defined(PRIME_OS_ANDROID)
    return UnixTime((int64_t)_stat.st_mtime, (int32_t)_stat.st_mtime_nsec);
#else
    return UnixTime((int64_t)_stat.st_mtim.tv_sec, (int32_t)_stat.st_mtim.tv_nsec);
#endif
}

void UnixFileProperties::setStatusChangeTime(const UnixTime& unixTime)
{
#ifdef PRIME_OS_QNX
    StaticCastAssign(_stat.st_ctime, unixTime.getSeconds());
#elif defined(PRIME_OS_ANDROID)
    StaticCastAssign(_stat.st_ctime, unixTime.getSeconds());
    StaticCastAssign(_stat.st_ctime_nsec, unixTime.getFractionNanoseconds());
#else
    StaticCastAssign(_stat.st_ctim.tv_sec, unixTime.getSeconds());
    StaticCastAssign(_stat.st_ctim.tv_nsec, unixTime.getFractionNanoseconds());
#endif
}

void UnixFileProperties::setLastAccessTime(const UnixTime& unixTime)
{
#ifdef PRIME_OS_QNX
    StaticCastAssign(_stat.st_atime, unixTime.getSeconds());
#elif defined(PRIME_OS_ANDROID)
    StaticCastAssign(_stat.st_atime, unixTime.getSeconds());
    StaticCastAssign(_stat.st_atime_nsec, unixTime.getFractionNanoseconds());
#else
    StaticCastAssign(_stat.st_atim.tv_sec, unixTime.getSeconds());
    StaticCastAssign(_stat.st_atim.tv_nsec, unixTime.getFractionNanoseconds());
#endif
}

void UnixFileProperties::setModificationTime(const UnixTime& unixTime)
{
#ifdef PRIME_OS_QNX
    StaticCastAssign(_stat.st_mtime, unixTime.getSeconds());
#elif defined(PRIME_OS_ANDROID)
    StaticCastAssign(_stat.st_mtime, unixTime.getSeconds());
    StaticCastAssign(_stat.st_mtime_nsec, unixTime.getFractionNanoseconds());
#else
    StaticCastAssign(_stat.st_mtim.tv_sec, unixTime.getSeconds());
    StaticCastAssign(_stat.st_mtim.tv_nsec, unixTime.getFractionNanoseconds());
#endif
}

bool UnixFileProperties::applyMode(const char* path, Log* log) const
{
    for (;;) {
        struct stat ss;
        memset(&ss, 0, sizeof(ss));
        if (::lstat(path, &ss) == 0) {
            break;
        }

        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }

    const mode_t wantBits = 0777 | S_ISUID | S_ISGID | S_ISVTX;

    // Now copy the relevant mode bits across.
    while (chmod(path, _stat.st_mode & wantBits) != 0) {
        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }

    return true;
}
}
