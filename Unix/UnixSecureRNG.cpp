// Copyright 2000-2021 Mark H. P. Lord

#include "UnixSecureRNG.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

// This will not work in a chroot jail...
// (Linux has a function for this now...)
#define RANDOM_PATH "/dev/urandom"

namespace Prime {

UnixSecureRNG::UnixSecureRNG()
    : _fd(-1)
{
}

UnixSecureRNG::~UnixSecureRNG()
{
    close();
}

bool UnixSecureRNG::init(Log* log)
{
    close();
    for (;;) {
        _fd = open(RANDOM_PATH, O_RDONLY | O_CLOEXEC);
        if (_fd >= 0) {
            return true;
        }

        if (errno != EINTR) {
            log->logErrno(errno, RANDOM_PATH);
            return false;
        }
    }
}

void UnixSecureRNG::close()
{
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
}

bool UnixSecureRNG::generateBytes(void* buffer, size_t bufferSize, Log* log)
{
    if (!isInitialised()) {
        if (!init(log)) {
            return false;
        }
    }

    for (;;) {
        ptrdiff_t nread = bufferSize ? read(_fd, buffer, bufferSize) : 0;
        if (nread < 0) {
            if (errno != EINTR) {
                log->logErrno(errno, RANDOM_PATH);
                return false;
            }

            // Retry EINTR.
        } else {
            bufferSize -= nread;
            if (!bufferSize) {
                return true;
            }

            buffer = (char*)buffer + nread;
        }
    }
}

UnixSecureRNG::Result UnixSecureRNG::generate()
{
    Result r;
    if (!generateBytes(&r, sizeof(r), Log::getGlobal())) {
        r = 0;
    }

    return r;
}
}
