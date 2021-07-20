// Copyright 2000-2021 Mark H. P. Lord

#include "UnixDirectoryReader.h"
#include "../Path.h"
#include "UnixCloseOnExec.h"
#include <errno.h>

namespace Prime {

UnixDirectoryReader::UnixDirectoryReader()
{
    _dir = NULL;
    _ent = NULL;
    _preventInherit = false;
}

UnixDirectoryReader::~UnixDirectoryReader()
{
    close();
}

bool UnixDirectoryReader::open(const char* path, Log* log, const Options& options)
{
    if (Path::hasTrailingSlashes(path)) {
        return open(Path::stripTrailingSlashes(path).c_str(), log, options);
    }

    if (!path || !*path) {
        path = ".";
    }

    close();
    _preventInherit = !options.getChildProcessInherit();

    if (_preventInherit) {
        UnixCloseOnExec::lock();
    }

    for (;;) {
        _dir = opendir(path);
        if (_dir) {
#ifdef PRIME_DIRENT_MISSING_D_TYPE
            _path = path;
#endif
            return true;
        }

        if (errno != EINTR) {
            if (_preventInherit) {
                UnixCloseOnExec::unlock();
            }
            log->logErrno(errno);
            return false;
        }
    }
}

void UnixDirectoryReader::close()
{
    if (_dir) {
        closedir(_dir);
        _dir = NULL;

        if (_preventInherit) {
            UnixCloseOnExec::unlock();
        }
    }

    _ent = NULL;
}

bool UnixDirectoryReader::read(Log*, bool* error)
{
    if (error) {
        *error = false;
    }

    if (!_dir) {
        return false;
    }

    for (;;) {
        _ent = readdir(_dir);

        if (_ent) {
            return true;
        }

        if (errno == EINTR) {
            continue;
        }

        close();
        return false;
    }
}

#ifdef PRIME_DIRENT_MISSING_D_TYPE
struct stat* UnixDirectoryReader::needStat() const
{
    PRIME_ASSERT(_ent);
    PRIME_ASSERT(!_path.empty());

    if (_stat.get()) {
        return _stat.get();
    }

    _stat.reset(new struct stat);

    char buffer[PRIME_BIG_STACK_BUFFER_SIZE];
    if (StringCopy(buffer, sizeof(buffer), _path)) {
        Path::joinInPlace(buffer, sizeof(buffer), _ent->d_name);

        if (::stat(buffer, _stat.get()) == 0) {
            return _stat.get();
        }
    }

    memset(_stat.get(), 0, sizeof(struct stat));
    return _stat.get();
}
#endif
}
