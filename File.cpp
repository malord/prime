// Copyright 2000-2021 Mark H. P. Lord

#include "File.h"
#include "DirectoryReader.h"
#include "FileProperties.h"
#include "FileStream.h"
#include "Path.h"
#include "PrefixLog.h"
#include "ScopedPtr.h"

namespace Prime {

#if !defined(PRIME_OS_WINDOWS) && !defined(PRIME_OS_UNIX)

bool FilenameMatch(const char* pattern, const char* string, bool)
{
    return WildcardMatch(pattern, string, true, Path::separators);
}

#endif

static bool MakePath(const char* path, Log* log, unsigned int permissions, bool toFile)
{
    ScopedArrayPtr<char> dup(NewString(path));

    if (toFile) {
        *(char*)Path::findLastComponent(dup.get()) = '\0';
    }

    for (char* p = dup.get();; ++p) {
        if (Path::isSlash(*p) || !*p) {
            if (p == dup.get()) {
                if (*p) {
                    continue;
                }

                // An empty path
                return true;
            }

            // Don't try to create "c:" or "/etc//" (note the trailing slashes)
            if (p != dup.get() && Path::isSeparator(p[-1])) {
                if (!*p) {
                    return true;
                }

                continue;
            }

            // Skip past any path separators so we can figure out if this is the final component of the path.
            const char* p2 = p;
            while (Path::isSeparator(*p2)) {
                ++p2;
            }

            bool isLastComponent = (*p2 == '\0');

            char pWas = *p;
            *p = 0;
            FileProperties fileProperties;
            if (!fileProperties.read(dup.get(), Log::getNullLog()) || !fileProperties.isDirectory()) {
                // Only report errors if we fail at the last component.
                // Note that the enclosing if statement uses isDirectory() to force an error for files that are in our way.
                if (!MakeDirectory(dup.get(), isLastComponent ? log : Log::getNullLog(), permissions)) {
                    if (isLastComponent) {
                        return false;
                    }
                }
            }
            if (!pWas) {
                return true;
            }

            *p = pWas;
        }
    }
}

bool MakePath(const char* path, Log* log, unsigned int permissions)
{
    return MakePath(path, log, permissions, false);
}

bool MakePathToFile(const char* path, Log* log, unsigned int permissions)
{
    return MakePath(path, log, permissions, true);
}

bool MakeTempName(char* pathTemplate)
{
    char* end = pathTemplate + strlen(pathTemplate);

    char* ptr = end;
    while (ptr-- != pathTemplate && *ptr == 'X') { }

    ++ptr;

    if (!PRIME_GUARD(*ptr == 'X')) {
        // pathTemplate does not end with Xs!
        return false;
    }

    while (ptr != end) {
        *ptr++ = (char)((rand() % 26) + 'A');
    }

    return true;
}

#if defined(PRIME_HAVE_DIRECTORYREADER) && defined(PRIME_HAVE_FILEPROPERTIES)

// Unix should really use fts() for this

bool RecursiveRemove(const char* path, Log* log)
{
    bool success = true;

    if (Path::hasTrailingSlashes(path)) {
        std::string withoutSlashes = Path::stripTrailingSlashes(path);
        return RecursiveRemove(withoutSlashes.c_str(), log);
    }

    FileProperties fileProperties;
    if (!fileProperties.readLink(path, log)) {
        return false;
    }

    if (fileProperties.isLink() || !fileProperties.isDirectory()) {
        return RemoveFile(path, log);
    }

    DirectoryReader dir;
    if (dir.open(path, log)) {
        while (dir.read(log)) {
            if (Path::isDotDirectory(dir.getName())) {
                continue;
            }

            std::string fullPath = Path::join(path, dir.getName());

            success = RecursiveRemove(fullPath.c_str(), log) && success;
        }
    } else {
        success = false;
    }

    return RemoveEmptyDirectory(path, log) && success;
}

#endif

bool CopyFileContents(const char* fromPath, const char* toPath, Log* log)
{
    PrefixLog fromLog(log, fromPath);
    FileStream from;
    if (!from.open(fromPath, OpenMode().setRead(), fromLog)) {
        return false;
    }

    Stream::Offset fromSize = from.getSize(fromLog);
    if (fromSize < 0) {
        return false;
    }

    PrefixLog toLog(log, toPath);
    FileStream to;
    if (!to.open(toPath, OpenMode().setOverwrite(), toLog)) {
        return false;
    }

    if (!to.copyFrom(&from, fromLog, fromSize, &toLog)) {
        return false;
    }

    return to.close(toLog);
}
}
