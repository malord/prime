// Copyright 2000-2021 Mark H. P. Lord

#include "Path.h"

namespace Prime {

#ifdef PRIME_DEBUG
template class BasicPath<UnixPathTraits>;
template class BasicPath<WindowsPathTraits>;
template class BasicPath<GenericPathTraits>;
template class BasicPath<PSPPathTraits>;
#endif

//
// PathTraitsBase
//

const char* PathTraitsBase::findInvalidCharacter(const char* begin, const char* end) PRIME_NOEXCEPT
{
    const unsigned char* ubegin = (const unsigned char*)begin;
    const unsigned char* uend = (const unsigned char*)end;

    for (; ubegin != uend; ++ubegin) {
        switch (*ubegin) {
        case '<':
        case '>':
        case ':':
        case '"':
        case '/':
        case '\\':
        case '|':
        case '?':
        case '*':
            return (const char*)ubegin;

        default:
            if (*ubegin < 32) {
                return (const char*)ubegin;
            }
            break;
        }
    }

    return (const char*)ubegin;
}

//
// WindowsPathTraits
//

const char* const WindowsPathTraits::separators = "\\:/";
const char* const WindowsPathTraits::slashes = "\\/";

// Should joining "c:windows" and "c:system" produce "c:windows\\system"? Right now, it does not.

bool WindowsPathTraits::isAbsolute(StringView path) PRIME_NOEXCEPT
{
    // Technically not true - a drive name can be followed by a relative path, but the path is absolute in
    // the sense that you can't append it to another path, e.g., c:\\windows\\c:system would be bad.
    if (hasDriveName(path)) {
        return true;
    }

    if (path.size() >= 2 && isSlash(path[0]) && isSlash(path[1])) {
        // UNC or "\\?\"
        return true;
    }

    return false;
}

size_t WindowsPathTraits::getRootLength(StringView path, bool& needsSlash, bool& hasSlash) PRIME_NOEXCEPT
{
    // TODO: check how this works with "\\?\" and "\\?\UNC\".

    size_t colonPos;
    if (hasDriveName(path, &colonPos)) {
        // We don't want a slash adding after a ':' because it changes the meaning of the path.
        needsSlash = false;
        hasSlash = colonPos + 1 < path.size() && isSlash(path[colonPos + 1]);
        return colonPos + 1 + (hasSlash ? 1 : 0);
    }

    // UNC?
    if (path.size() >= 2 && isSlash(path[0]) && isSlash(path[1])) {
        const char* ptr = path.begin() + 2;
        while (ptr != path.end() && !isSlash(*ptr)) {
            ++ptr;
        }

        if (ptr != path.end()) {
            do {
                ++ptr;
            } while (ptr != path.end() && !isSlash(*ptr));

            if (ptr == path.end()) {
                needsSlash = true;
                hasSlash = false;
                return ptr - path.begin();
            } else {
                needsSlash = false;
                hasSlash = true;
                return ptr - path.begin() + 1;
            }
        }
    }

    needsSlash = false;
    hasSlash = false;
    return 0;
}

bool WindowsPathTraits::hasDriveName(StringView path, size_t* optionalColonPos) PRIME_NOEXCEPT
{
    const char* ptr = path.begin();

    for (;;) {
        if (ptr == path.end() || isSlash(*ptr)) {
            return false;
        }

        if (*ptr == ':') {
            if (optionalColonPos) {
                *optionalColonPos = (size_t)(ptr - path.begin());
            }

            return true;
        }

        ++ptr;
    }
}

//
// UnixPathTraits
//

const char* const UnixPathTraits::separators = "\\:/";
const char* const UnixPathTraits::slashes = "/\\";

size_t UnixPathTraits::getRootLength(StringView path, bool& needsSlash, bool& hasSlash) PRIME_NOEXCEPT
{
    if (!path.empty() && isSeparator(path[0])) {
        needsSlash = false;
        hasSlash = true;
        return 1;
    }

    needsSlash = true;
    hasSlash = false;
    return 0;
}

//
// GenericPathTraits
//

const char* const GenericPathTraits::separators = "\\:/";
const char* const GenericPathTraits::slashes = "/\\";

bool GenericPathTraits::isAbsolute(StringView path) PRIME_NOEXCEPT
{
    // Many platforms allow a device/mount point prefix (e.g., game: or app0:)
    if (hasMountPoint(path)) {
        return true;
    }

    return !path.empty() && isSlash(path[0]);
}

size_t GenericPathTraits::getRootLength(StringView path, bool& needsSlash, bool& hasSlash) PRIME_NOEXCEPT
{
    size_t colonPos;
    if (hasMountPoint(path, &colonPos)) {
        // We don't want a slash adding after a ':' because it changes the meaning of the path.
        needsSlash = false;
        hasSlash = colonPos + 1 < path.size() && isSlash(path[colonPos + 1]);
        return colonPos + 1 + (hasSlash ? 1 : 0);
    }

    if (!path.empty() && isSlash(path[0])) {
        needsSlash = false;
        hasSlash = true;
        return 1;
    }

    needsSlash = true;
    hasSlash = false;
    return 0;
}

//
// PSPPathTraits
//

bool PSPPathTraits::isAbsolute(StringView path) PRIME_NOEXCEPT
{
    const char* ptr = path.begin();
    for (;;) {
        if (ptr == path.end() || *ptr == '/') {
            return false;
        }

        if (*ptr == ':') {
            return ptr + 1 != path.end() && ptr[1] == '/';
        }

        ++ptr;
    }
}

bool PSPPathTraits::isRootRelative(StringView path) PRIME_NOEXCEPT
{
    return !path.empty() && path[0] == '/';
}

size_t PSPPathTraits::getRootLength(StringView path, bool& needsSlash, bool& hasSlash) PRIME_NOEXCEPT
{
    const char* ptr = path.begin();
    for (;;) {
        if (ptr == path.end() || *ptr == '/') {
            break;
        }

        if (*ptr == ':') {
            hasSlash = ptr + 1 != path.end() && ptr[1] == '/';
            needsSlash = false;
            return (ptr - path.begin()) + (hasSlash ? 2 : 1);
        }

        ++ptr;
    }

    needsSlash = hasSlash = false;
    return 0;
}
}
