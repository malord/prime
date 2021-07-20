// Copyright 2000-2021 Mark H. P. Lord

#include "FileSystem.h"

namespace Prime {

PRIME_DEFINE_UID_CAST_BASE(FileSystem::FileProperties)
PRIME_DEFINE_UID_CAST_BASE(FileSystem::OpenOptions)
PRIME_DEFINE_UID_CAST_BASE(FileSystem::DirectoryReader)
PRIME_DEFINE_UID_CAST_BASE(FileSystem)

FileSystem::~FileSystem()
{
}

bool FileSystem::test(const char* path, FileProperties* fileProperties)
{
    // Zero openMode must be supported by all FileSystems.
    return open(path, OpenMode(), Log::getNullLog(), OpenOptions(), fileProperties).get() != NULL;
}

RefPtr<FileSystem::DirectoryReader> FileSystem::readDirectory(const char* path, Log* log)
{
    log->error(PRIME_LOCALISE("FileSystem cannot provide directory for: %s"), path);
    return NULL;
}

bool FileSystem::remove(const char*, Log* log)
{
    log->error(PRIME_LOCALISE("FileSystem does not support remove()."));
    return false;
}

bool FileSystem::rename(const char*, const char*, Log* log, bool)
{
    log->error(PRIME_LOCALISE("FileSystem does not support rename()."));
    return false;
}

bool FileSystem::getSystemPath(std::string&, const char*, FileProperties*)
{
    return false;
}

//
// FileSystem::DirectoryReader
//

FileSystem::DirectoryReader::~DirectoryReader()
{
}

bool FileSystem::DirectoryReader::isHidden() const
{
    return false;
}

bool FileSystem::DirectoryReader::isLink() const
{
    return false;
}

bool FileSystem::DirectoryReader::isFile() const
{
    return !isDirectory() && !isLink();
}
}
