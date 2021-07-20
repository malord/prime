// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_LOGGINGFILESYSTEM_H
#define PRIME_LOGGINGFILESYSTEM_H

#include "FileSystem.h"
#include <string>

namespace Prime {

/// Wraps a FileSystem and writes logs as files are opened.
class PRIME_PUBLIC LoggingFileSystem : public FileSystem {
public:
    void init(const char* prefix, FileSystem* fileSystem, Log* log, Log::Level logLevel);

    // FileSystem implementation.
    virtual RefPtr<Stream> open(const char* path, const OpenMode& openMode, Log* log,
        const OpenOptions& openOptions,
        FileProperties* fileProperties) PRIME_OVERRIDE;
    virtual bool test(const char* path, FileProperties* fileProperties = NULL) PRIME_OVERRIDE;
    virtual bool remove(const char* path, Log* log) PRIME_OVERRIDE;
    virtual bool rename(const char* from, const char* to, Log* log, bool overwrite = false) PRIME_OVERRIDE;
    virtual RefPtr<DirectoryReader> readDirectory(const char* path, Log* log) PRIME_OVERRIDE;
    virtual bool getSystemPath(std::string& systemPath, const char* path, FileProperties* fileProperties = NULL) PRIME_OVERRIDE;

private:
    std::string _prefix;
    RefPtr<FileSystem> _fileSystem;
    RefPtr<Log> _log;
    Log::Level _logLevel;
};
}

#endif
