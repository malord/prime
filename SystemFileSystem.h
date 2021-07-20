// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SYSTEMFILESYSTEM_H
#define PRIME_SYSTEMFILESYSTEM_H

#include "FileSystem.h"
#include "ScopedPtr.h"

namespace Prime {

/// A FileSystem implementation for the system file system. UNIX style forward slashes are always the path
/// separator, although Windows slashes are also supported on Windows.
class PRIME_PUBLIC SystemFileSystem : public FileSystem {
public:
    SystemFileSystem() { }

    SystemFileSystem(const char* path)
    {
        setPath(path);
    }

    void setPath(const char* path);

    void computeFullPath(std::string& fullPath, const char* path) const;

    // FileSystem implementation.
    virtual RefPtr<Stream> open(const char* path, const OpenMode& openMode, Log* log,
        const OpenOptions& openOptions,
        FileProperties* fileProperties = NULL) PRIME_OVERRIDE;
    virtual RefPtr<DirectoryReader> readDirectory(const char* path, Log* log) PRIME_OVERRIDE;
    virtual bool remove(const char* path, Log* log) PRIME_OVERRIDE;
    virtual bool rename(const char* from, const char* to, Log* log, bool overwrite = false) PRIME_OVERRIDE;
    virtual bool getSystemPath(std::string& systemPath, const char* path, FileProperties* fileProperties = NULL) PRIME_OVERRIDE;

private:
    std::string _path;

    PRIME_UNCOPYABLE(SystemFileSystem);
};
}

#endif
