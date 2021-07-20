// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PREFIXFILESYSTEM_H
#define PRIME_PREFIXFILESYSTEM_H

#include "FileSystem.h"
#include <string>

namespace Prime {

/// Forward all calls to an underlying FileSystem with a prefix joined on all paths.
class PRIME_PUBLIC PrefixFileSystem : public FileSystem {
public:
    PrefixFileSystem();

    PrefixFileSystem(FileSystem* fileSystem, const char* prefix);

    ~PrefixFileSystem();

    /// A prefix of "folder" would result in open("test.txt") opening "folder/test.txt".
    void init(FileSystem* fileSystem, const char* prefix);

    // FileSystem implementation.
    virtual RefPtr<Stream> open(const char* path, const OpenMode& openMode, Log* log,
        const OpenOptions& openOptions = OpenOptions(),
        FileProperties* fileProperties = NULL) PRIME_OVERRIDE;
    virtual bool test(const char* path, FileProperties* fileProperties = NULL) PRIME_OVERRIDE;
    virtual bool remove(const char* path, Log* log) PRIME_OVERRIDE;
    virtual bool rename(const char* from, const char* to, Log* log, bool overwrite = false) PRIME_OVERRIDE;
    virtual RefPtr<DirectoryReader> readDirectory(const char* path, Log* log) PRIME_OVERRIDE;
    virtual bool getSystemPath(std::string& systemPath, const char* path, FileProperties* fileProperties = NULL) PRIME_OVERRIDE;

private:
    RefPtr<FileSystem> _fileSystem;
    std::string _prefix;
};
}

#endif
