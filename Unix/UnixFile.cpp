// Copyright 2000-2021 Mark H. P. Lord

#include "../File.h"
#include "../Path.h"
#include "../ScopedPtr.h"
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace Prime {

namespace {

    inline const char* SkipSlashes(const char* path)
    {
        return ASCIISkipChar(path, '/');
    }
}

bool FileExists(const char* path, Log* log)
{
    (void)log;
    return access(path, 0) == 0;
}

bool RenameFile(const char* from, const char* to, Log* log)
{
    while (::rename(from, to) != 0) {
        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }

    return true;
}

bool RenameFileOverwrite(const char* from, const char* to, Log* log)
{
    return RenameFile(from, to, log);
}

bool RemoveFile(const char* path, Log* log)
{
    while (::unlink(path) != 0) {
        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }

    return true;
}

bool MakeDirectory(const char* path, Log* log, unsigned int permissions)
{
    while (::mkdir(path, (mode_t)permissions) != 0) {
        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }

    return true;
}

bool RemoveEmptyDirectory(const char* path, Log* log)
{
    while (::rmdir(path) != 0) {
        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }

    return true;
}

bool GetWorkingDirectory(std::string& out, Log* log)
{
    for (;;) {
        char* cwd = ::getcwd(0, 0);

        if (!cwd) {
            if (errno == EINTR) {
                continue;
            }

            log->logErrno(errno);
            return false;
        }

        out.assign(cwd);

        free(cwd);

        return true;
    }
}

bool SetWorkingDirectory(const char* path, Log* log)
{
    while (chdir(path) != 0) {
        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }

    return true;
}

bool FilenameMatch(const char* pattern, const char* string, bool forceCaseFold)
{
#ifdef FNM_CASEFOLD
    if (forceCaseFold) {
        return fnmatch(pattern, string, FNM_PATHNAME | FNM_CASEFOLD) == 0;
    }
#endif

    (void)forceCaseFold;

    return fnmatch(pattern, string, FNM_PATHNAME) == 0;
}

bool NormalisePath(std::string& normalised, const char* path, Log* log)
{
    // First, make an absolute path. realpath() does this, but only for paths that already exist, and I want to
    // support paths that don't yet exist.
    std::string fullPath;
    if (*path != '/') {
        if (!GetWorkingDirectory(fullPath, log)) {
            return false;
        }

        fullPath += '/';
        fullPath += path;
        path = fullPath.c_str();
    }

    std::string output;
    const char* inputPath = path;

    // Start with the root directory.
    if (*path == '/') {
        output += '/';
        path = SkipSlashes(path + 1);
    }

    // See how much of the path can be accessed with realpath(). This gives us the canonical name of as much of
    // the path as exists. (It would be faster just to do the test backwards!)
    const char* rpathEnd = path;
    for (;;) {
        while (*rpathEnd && *rpathEnd != '/') {
            ++rpathEnd;
        }

        std::string partialPath(inputPath, rpathEnd);
        char rpath[PATH_MAX];
        if (!realpath(partialPath.c_str(), rpath)) {
            break;
        }

        output = rpath;
        path = rpathEnd;

        rpathEnd = SkipSlashes(rpathEnd);
        if (!*rpathEnd) {
            // Processed the entire path.
            normalised = output;
            return true;
        }
    }

    // path now points to the remainder of the path which realpath() couldn't find.
    // output now contains the path of the path which realpath() could find.

    if (*path == '/') {
        output += '/';
        path = SkipSlashes(path + 1);
    }

    PRIME_ASSERT(*path); // We should have returned already.

    size_t rootLength = (!output.empty() && output[0] == '/') ? 1 : 0;

    // Now proceed to expand the remaining path one step at a time, checking for duplicate slashes.
    for (;;) {
        const char* nameEnd = path;
        while (*nameEnd && *nameEnd != '/') {
            ++nameEnd;
        }

        if (path[0] == '.') {
            // Ignore "."
            if (nameEnd == path + 1) {
                path = SkipSlashes(nameEnd);
                if (!*path) {
                    break;
                }

                continue;
            }

            // Process ".."
            if (path[1] == '.' && nameEnd == path + 2) {
                // output.size() > 1, rather than >= 1, because I don't want the root removed.
                if (output.size() > 1 && output.back() == '/') {
                    output.pop_back();
                }

                size_t last = output.rfind('/');

                if (last == std::string::npos || last < rootLength) {
                    output.resize(rootLength); // This shouldn't happen if we had an absolute path to begin with
                } else {
                    output.resize(last + 1);
                }

                path = SkipSlashes(nameEnd);
                if (!*path) {
                    break;
                }

                continue;
            }
        }

        output.append(path, nameEnd);

        if (*nameEnd) {
            path = SkipSlashes(nameEnd + 1);
        } else {
            path = nameEnd;
        }

        if (!*path) {
            break;
        }

        output += '/';
    }

    // Eliminate any trailing slash.
    if (output.size() > rootLength && output.back() == '/') {
        output.pop_back();
    }

    normalised = output;
    return true;
}

bool IsSameFile(const char* a, const char* b, Log* log)
{
    struct ::stat stata;
    struct ::stat statb;

    if (::stat(a, &stata) == 0 && ::stat(b, &statb) == 0) {
        return stata.st_dev == statb.st_dev && stata.st_ino == statb.st_ino;
    }

    // Normalise the names and try for filename comparison.

    std::string norma;
    if (!NormalisePath(norma, a, log)) {
        norma = a;
    }

    std::string normb;
    if (!NormalisePath(normb, b, log)) {
        normb = b;
    }

    return norma == normb;
}

bool SetUnixFileMode(const char* path, unsigned int mode, Log* log)
{
    while (chmod(path, (mode_t)mode) != 0) {
        if (errno != EINTR) {
            log->logErrno(errno);
            return false;
        }
    }

    return true;
}

// This code may yet work on Linux/BSD, so I'm keeping it here
#ifdef PRIME_OS_OSX

bool IncreaseMaxFileDescriptors(Log* log, int requiredDescriptors)
{
    rlimit rl;
    memset(&rl, 0, sizeof(rl));
    if (getrlimit(RLIMIT_NOFILE, &rl) == -1) {
        log->logErrno(errno, "Unable to determine available file handles.");
        return false;
    }

    log->trace("Current file descriptors: %ld max: %ld", static_cast<long>(rl.rlim_cur), static_cast<long>(rl.rlim_max));

    rl.rlim_cur = std::min<decltype(rl.rlim_max)>(requiredDescriptors < 0 ? OPEN_MAX : static_cast<decltype(rl.rlim_max)>(requiredDescriptors), rl.rlim_max);

    if (setrlimit(RLIMIT_NOFILE, &rl) == -1) {
        log->logErrno(errno, "Unable to increase available file handles.");
        return false;
    }

    memset(&rl, 0, sizeof(rl));
    getrlimit(RLIMIT_NOFILE, &rl);

    log->trace("Max file descriptors now: %ld", static_cast<long>(rl.rlim_cur));
    return true;
}

#endif
}
