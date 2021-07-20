// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsWildcardExpansion.h"
#include "../Path.h"

namespace Prime {

WindowsWildcardExpansion::WindowsWildcardExpansion()
{
    construct();
}

void WindowsWildcardExpansion::construct()
{
    _begun = false;
}

WindowsWildcardExpansion::WindowsWildcardExpansion(const char* pattern, const Options& options, Log* log)
{
    construct();
    find(pattern, options, log);
}

WindowsWildcardExpansion::~WindowsWildcardExpansion()
{
    close();
}

bool WindowsWildcardExpansion::find(const char* pattern, const Options& options, Log* log)
{
    close();

    _pattern = pattern;

    // If you ask for C:\\WINDOWS\\ you'll get C:\\WINDOWS.
    WindowsPath::stripTrailingSlashesInPlace(_pattern);

    _options = options;

    _path = _pattern;
    Path::stripLastComponentInPlace(_path);
    Path::stripTrailingSlashesInPlace(_path);

    _wildcard = Path::findLastComponent(_pattern.c_str());

    _joinedToUse = 0;

    bool fail = options.getFailIfNoMatches();

    if (!_dir.openWildcard(_pattern.c_str(), fail ? log : (Log*)Log::getNullLog())) {
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

const char* WindowsWildcardExpansion::read(Log* log)
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

bool WindowsWildcardExpansion::findNextMatch(Log* log)
{
    PRIME_ASSERT(_dirOpen);

    for (;;) {
        if (!_dir.read(log)) {
            _dir.close();
            _dirOpen = false;
            return false;
        }

        if (_options.getExcludeHiddenFiles() && _dir.isHidden()) {
            continue;
        }

        if (Path::isDotDirectory(_dir.getName())) {
            continue;
        }

        _joined[_joinedToUse] = Path::join(_path, _dir.getName());
        _next = _joined[_joinedToUse].c_str();
        _joinedToUse ^= 1;
        return true;
    }
}

void WindowsWildcardExpansion::close()
{
    if (!_begun) {
        return;
    }

    _dir.close();
    _begun = false;
}
}
