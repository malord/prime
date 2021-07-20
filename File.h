// Copyright 2000-2021 Mark H. P. Lord

// Under OS X, file names are UTF-8 but they're decomposed. It might be useful to have a function to convert
// a file name to NFC (using CFStringCreateWithFileSystemRepresentation and CFStringGetFileSystemRepresentation).
// (This same function would also be useful in cases where the filesystem has a non-Unicode encoding.)

#ifndef PRIME_FILE_H
#define PRIME_FILE_H

#include "Log.h"
#include <string>

namespace Prime {

//
// File system operations
//

PRIME_PUBLIC bool FileExists(const char* path, Log* log);

PRIME_PUBLIC bool RenameFile(const char* from, const char* to, Log* log);

/// Like RenameFile, but overwrite to if it already exists. This is atomic on UNIX (and Windows XP+).
PRIME_PUBLIC bool RenameFileOverwrite(const char* from, const char* to, Log* log);

PRIME_PUBLIC bool RemoveFile(const char* path, Log* log);

PRIME_PUBLIC bool MakeDirectory(const char* path, Log* log, unsigned int permissions = 0777);

/// Make a directory and any directories along the path.
PRIME_PUBLIC bool MakePath(const char* path, Log* log, unsigned int permissions = 0777);

/// Make a directory and any directories along the path, excluding the last path component.
PRIME_PUBLIC bool MakePathToFile(const char* path, Log* log, unsigned int permissions = 0777);

PRIME_PUBLIC bool RemoveEmptyDirectory(const char* path, Log* log);

/// If path is a file, remove it. If path is a directory, recursively remove all the directories and files it
/// contains, then remove path itself.
PRIME_PUBLIC bool RecursiveRemove(const char* path, Log* log);

PRIME_PUBLIC bool GetWorkingDirectory(std::string& cwd, Log* log);

PRIME_PUBLIC bool SetWorkingDirectory(const char* path, Log* log);

PRIME_PUBLIC bool FilenameMatch(const char* pattern, const char* string, bool forceCaseFold = false);

#ifdef PRIME_OS_WINDOWS

PRIME_PUBLIC bool GetWindowsFileAttributes(uint32_t& attributes, const char* path, Log* log);

PRIME_PUBLIC bool SetWindowsFileAttributes(const char* path, uint32_t attributes, Log* log);

/// Removes the hidden, system and read-only attributes from a file.
PRIME_PUBLIC bool ClearHiddenSystemReadOnlyAttributes(const char* path, Log* log);

#endif

#ifdef PRIME_OS_UNIX

/// e.g., SetUnixFileMode(path, 0777, log);
PRIME_PUBLIC bool SetUnixFileMode(const char* path, unsigned int mode, Log* log);

#endif

/// Normalises a path, attempting to determine its canonical name (in terms of both location and character case).
/// The path does not need to exist. As much as possible of the path will be expanded using operating system APIs
/// (e.g., realpath() or GetFullPathName()), then the remainder of the path will be appended dealing with
/// repeated slashes (e.g., "path///path" => "path/path"), "." and "..". All slashes will also be converted to
/// the operating system norm.
/// On Windows, drive letters are expanded, but no attempt is made to resolve network shares to local paths.
/// On Unix, all symlinks are expanded.
PRIME_PUBLIC bool NormalisePath(std::string& normalised, const char* path, Log* log);

/// Returns true if paths pathA and pathB definitely refer to the same file. Uses stat() on UNIX to compare
/// inodes, otherwise compares file paths (resolving symlinks).
PRIME_PUBLIC bool IsSameFile(const char* pathA, const char* pathB, Log* log);

/// You supply a string that ends with Xs, e.g., myapp-XXXXXX, and the Xs will be replaced for you with a
/// randomish sequence of characters. Does not check that the file does not exist (i.e., this is not mktemp,
/// it's just mktemp's file naming logic). Prefer to use the TempFile and TempDirectory classes.
PRIME_PUBLIC bool MakeTempName(char* pathTemplate);

/// Performs a copy of the contents of one file to another. Metadata (file mode, timestamps, ACLs etc.) are lost.
PRIME_PUBLIC bool CopyFileContents(const char* fromPath, const char* toPath, Log* log);

/// Recursively removes a file or directory when destructed (unless cancel() is called).
class ScopedRecursiveRemove {
public:
    ScopedRecursiveRemove() { }

    /// Immediately calls init().
    ScopedRecursiveRemove(const char* path, Log* log) { init(path, log); }

    ~ScopedRecursiveRemove() { recursiveRemove(); }

    void init(const char* path, Log* log)
    {
        PRIME_ASSERT(!isInitialised());
        _path = path;
        _log = log;
    }

    bool isInitialised() const { return !_path.empty(); }

    /// Detach the path from this object, so it won't be removed.
    void cancel() { _path.resize(0); }

    /// Recursively remove the path.
    void recursiveRemove()
    {
        if (_path.empty()) {
            return;
        }

        RecursiveRemove(_path.c_str(), _log);

        cancel();
    }

private:
    std::string _path;
    RefPtr<Log> _log;

    PRIME_UNCOPYABLE(ScopedRecursiveRemove);
};

//
// System utilities
//

#ifdef PRIME_OS_OSX

#define PRIME_HAVE_INCREASEMAXFILEDESCRIPTORS

bool IncreaseMaxFileDescriptors(Log* log, int requiredDescriptors = -1);

#endif

}

#endif
