// Copyright 2000-2021 Mark H. P. Lord

#include "PrefixFileSystem.h"
#include "Path.h"

namespace Prime {

PrefixFileSystem::PrefixFileSystem()
{
}

PrefixFileSystem::PrefixFileSystem(FileSystem* fileSystem, const char* prefix)
{
    init(fileSystem, prefix);
}

PrefixFileSystem::~PrefixFileSystem()
{
}

void PrefixFileSystem::init(FileSystem* fileSystem, const char* prefix)
{
    _fileSystem = fileSystem;
    _prefix = prefix;
}

bool PrefixFileSystem::test(const char* path, FileProperties* fileProperties)
{
    std::string fullPath = UnixPath::join(_prefix, path);

    return _fileSystem->test(fullPath.c_str(), fileProperties);
}

RefPtr<Stream> PrefixFileSystem::open(const char* path, const OpenMode& openMode, Log* log,
    const OpenOptions& openOptions, FileProperties* fileProperties)
{
    std::string fullPath = UnixPath::join(_prefix, path);

    return _fileSystem->open(fullPath.c_str(), openMode, log, openOptions, fileProperties);
}

bool PrefixFileSystem::remove(const char* path, Log* log)
{
    std::string fullPath = UnixPath::join(_prefix, path);

    return _fileSystem->remove(fullPath.c_str(), log);
}

bool PrefixFileSystem::rename(const char* from, const char* to, Log* log, bool overwrite)
{
    std::string fullTo = UnixPath::join(_prefix, to);
    std::string fullFrom = UnixPath::join(_prefix, from);

    return _fileSystem->rename(fullFrom.c_str(), fullTo.c_str(), log, overwrite);
}

RefPtr<FileSystem::DirectoryReader> PrefixFileSystem::readDirectory(const char* path, Log* log)
{
    std::string fullPath = UnixPath::join(_prefix, path);

    return _fileSystem->readDirectory(fullPath.c_str(), log);
}

bool PrefixFileSystem::getSystemPath(std::string& systemPath, const char* path, FileProperties* fileProperties)
{
    std::string fullPath = UnixPath::join(_prefix, path);

    return _fileSystem->getSystemPath(systemPath, fullPath.c_str(), fileProperties);
}
}
