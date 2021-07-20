// Copyright 2000-2021 Mark H. P. Lord

#include "EmulatedWildcardExpansion.h"

#ifdef PRIME_HAVE_EMULATEDWILDCARDEXPANSION

#define USE_FILENAMEMATCH
#ifdef USE_FILENAMEMATCH
// FilenameMatch uses WildcardMatch on platforms which don't have a filename matching API
#include "../File.h"
#endif
#include "../Path.h"

namespace Prime {

EmulatedWildcardExpansion::EmulatedWildcardExpansion()
{
    construct();
}

void EmulatedWildcardExpansion::construct()
{
    _begun = false;
}

EmulatedWildcardExpansion::EmulatedWildcardExpansion(const char* pattern, const Options& options, Log* log)
{
    construct();
    find(pattern, options, log);
}

EmulatedWildcardExpansion::~EmulatedWildcardExpansion()
{
    close();
}

bool EmulatedWildcardExpansion::find(const char* pattern, const Options& options, Log* log)
{
    close();

    _pattern = pattern;

    // If you ask for C:\\WINDOWS\\ you'll get C:\\WINDOWS (but if you ask for C:\\ you'll still get C:\\).
    Path::stripTrailingSlashesInPlace(_pattern);

    _options = options;

    _path = _pattern;
    Path::stripLastComponentInPlace(_path);
    Path::stripTrailingSlashesInPlace(_path);

    _wildcard = Path::findLastComponent(_pattern.c_str());

    _joinedToUse = 0;

    bool fail = options.getFailIfNoMatches();

    if (!_dir.open(_path.c_str(), fail ? log : Log::getNullLog())) {
        if (fail) {
            return false;
        }

        _next = _pattern.c_str();
        _dirOpen = false;
        _begun = true;
        return true;
    }

    _dirOpen = true;

    if (!findNextMatch(log)) {
        if (fail) {
            return false;
        }

        _next = _pattern.c_str();
        _begun = true;
        return true;
    }

    _begun = true;
    return true;
}

const char* EmulatedWildcardExpansion::read(Log* log)
{
    PRIME_ASSERT(_begun);

    if (!_next) {
        return 0;
    }

    const char* got = _next;
    _next = 0;

    if (_dirOpen) {
        findNextMatch(log);
    }

    return got;
}

bool EmulatedWildcardExpansion::findNextMatch(Log* log)
{
    PRIME_ASSERT(_dirOpen);

    for (;;) {
        if (!_dir.read(log)) {
            _dir.close();
            _dirOpen = false;
            return false;
        }

#ifdef USE_FILENAMEMATCH
        if (!FilenameMatch(_wildcard, _dir.getName())) {
#else
        if (!WildcardMatch(_wildcard, _dir.getName(), true, Path::separators)) {
#endif

            continue;
        }

        if (_options.getExcludeHiddenFiles() && _dir.isHidden()) {
            continue;
        }

        _joined[_joinedToUse] = Path::join(_path, _dir.getName());
        _next = _joined[_joinedToUse].c_str();
        _joinedToUse ^= 1;
        return true;
    }
}

void EmulatedWildcardExpansion::close()
{
    if (!_begun) {
        return;
    }

    _dir.close();
    _begun = false;
}
}

#endif
