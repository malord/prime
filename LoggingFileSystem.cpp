// Copyright 2000-2021 Mark H. P. Lord

#include "LoggingFileSystem.h"

namespace Prime {

void LoggingFileSystem::init(const char* prefix, FileSystem* fileSystem, Log* log, Log::Level logLevel)
{
    _prefix = prefix;
    _fileSystem = fileSystem;
    _log = log;
    _logLevel = logLevel;
}

RefPtr<Stream> LoggingFileSystem::open(const char* path, const OpenMode& openMode, Log* log,
    const OpenOptions& openOptions, FileProperties* fileProperties)
{
    RefPtr<Stream> stream = _fileSystem->open(path, openMode, log, openOptions, fileProperties);

    _log->log(_logLevel, "%s%s: %s", _prefix.c_str(), stream ? "Opened" : "NOT FOUND", path);

    return stream;
}

bool LoggingFileSystem::test(const char* path, FileProperties* fileProperties)
{
    return _fileSystem->test(path, fileProperties);
}

bool LoggingFileSystem::remove(const char* path, Log* log)
{
    return _fileSystem->remove(path, log);
}

bool LoggingFileSystem::rename(const char* from, const char* to, Log* log, bool overwrite)
{
    return _fileSystem->rename(from, to, log, overwrite);
}

RefPtr<FileSystem::DirectoryReader> LoggingFileSystem::readDirectory(const char* path, Log* log)
{
    RefPtr<DirectoryReader> dr = _fileSystem->readDirectory(path, log);

    return dr;
}

bool LoggingFileSystem::getSystemPath(std::string& systemPath, const char* path, FileProperties* fileProperties)
{
    return _fileSystem->getSystemPath(systemPath, path, fileProperties);
}
}
