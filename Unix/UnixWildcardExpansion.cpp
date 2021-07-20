// Copyright 2000-2021 Mark H. P. Lord

#include "UnixWildcardExpansion.h"

#ifndef PRIME_OS_ANDROID

#include <errno.h>
#include <string.h>

namespace Prime {

UnixWildcardExpansion::UnixWildcardExpansion()
{
    construct();
}

void UnixWildcardExpansion::construct()
{
    _globbed = false;
}

UnixWildcardExpansion::UnixWildcardExpansion(const char* pattern, const Options& options, Log* log)
{
    construct();
    find(pattern, options, log);
}

UnixWildcardExpansion::~UnixWildcardExpansion()
{
    close();
}

bool UnixWildcardExpansion::find(const char* pattern, const Options& options, Log* log)
{
    close();

    memset(&_globStruct, 0, sizeof(_globStruct));

    int globFlags = GLOB_TILDE;

    if (!options.getSort()) {
        globFlags |= GLOB_NOSORT;
    }

    if (!options.getFailIfNoMatches()) {
        globFlags |= GLOB_NOCHECK;
    }

    for (;;) {
        int result = glob(pattern, globFlags, 0, &_globStruct);

        if (result != 0) {
            if (result == GLOB_NOMATCH) {
                errno = ENOENT;
            }

            if (errno == EINTR) {
                continue;
            }

            log->logErrno(errno);
            return false;
        }

        _globbed = true;
        _next = 0;

        return true;
    }
}

const char* UnixWildcardExpansion::read(Log*)
{
    PRIME_ASSERT(_globbed);

    if (_next == (ptrdiff_t)_globStruct.gl_pathc) {
        return 0;
    }

    const char* got = _globStruct.gl_pathv[_next];
    ++_next;

    return got;
}

void UnixWildcardExpansion::close()
{
    if (_globbed) {
        globfree(&_globStruct);

        _globbed = false;
    }
}
}

#endif // PRIME_OS_ANDROID

