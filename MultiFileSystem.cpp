// Copyright 2000-2021 Mark H. P. Lord

#include "MultiFileSystem.h"
#include <algorithm>

namespace Prime {

void MultiFileSystem::reset()
{
    if (_readableCount) {
        std::fill(_readable, _readable + _readableCount, RefPtr<FileSystem>());
        _readableCount = 0;
    }

    _writable.release();
}

void MultiFileSystem::addFileSystem(FileSystem* fileSystem)
{
    if (_readableCount < PRIME_COUNTOF(_readable) - 1) {
        _readable[_readableCount++] = fileSystem;
        return;
    }

    RefPtr<FileSystem>& lastReadable = _readable[PRIME_COUNTOF(_readable) - 1];
    if (!lastReadable) {
        lastReadable = PassRef(new MultiFileSystem);
        _readableCount++;
    }

    PRIME_DEBUG_ASSERT(_readableCount == PRIME_COUNTOF(_readable));

    static_cast<MultiFileSystem*>(lastReadable.get())->addFileSystem(fileSystem);
}

void MultiFileSystem::setWritableFileSystem(FileSystem* fileSystem)
{
    _writable = fileSystem;
}

bool MultiFileSystem::test(const char* path, FileProperties* fileProperties)
{
    for (size_t i = 0; i != _readableCount; ++i) {
        if (_readable[i]->test(path, fileProperties)) {
            return true;
        }
    }

    if (_writable) {
        return _writable->test(path, fileProperties);
    }

    return false;
}

RefPtr<Stream> MultiFileSystem::open(const char* path, const OpenMode& openMode, Log* log,
    const OpenOptions& openOptions, FileProperties* fileProperties)
{
    if (openMode.isWriteAccessRequired()) {
        if (!_writable) {
            log->error(PRIME_LOCALISE("%s: Writing not supported."), path);
            return NULL;
        }

        return _writable->open(path, openMode, log, openOptions, fileProperties);
    }

    if (!_readableCount) {
        log->error(PRIME_LOCALISE("%s: No locations from which to open files."), path);
        return NULL;
    }

    for (size_t i = 0; i != _readableCount; ++i) {
        FileProperties readableProps;
        RefPtr<Stream> stream = _readable[i]->open(path, openMode, Log::getNullLog(), openOptions, &readableProps);
        if (stream) {
            if (fileProperties) {
                *fileProperties = readableProps;
            }

            return stream;
        }
    }

    // Let the first FileSystem report the error for us.
    return _readable[0]->open(path, openMode, log, openOptions, fileProperties);
}

bool MultiFileSystem::remove(const char* path, Log* log)
{
    if (_writable) {
        return _writable->remove(path, log);
    }

    log->error(PRIME_LOCALISE("%s: Read only file system."), path);
    return false;
}

bool MultiFileSystem::rename(const char* from, const char* to, Log* log, bool overwrite)
{
    if (_writable) {
        return _writable->rename(from, to, log, overwrite);
    }

    log->error(PRIME_LOCALISE("%s: Read only file system."), to);
    return false;
}

RefPtr<FileSystem::DirectoryReader> MultiFileSystem::readDirectory(const char* path, Log* log)
{
    // THIS IS VERY WRONG. It needs to merge the results of all the DirectoryReaders which can read the directory.

    for (size_t i = 0; i != _readableCount; ++i) {
        RefPtr<FileSystem::DirectoryReader> dr = _readable[i]->readDirectory(path, Log::getNullLog());
        if (dr) {
            return dr;
        }
    }

    if (_writable) {
        return _writable->readDirectory(path, log);
    }

    if (_readableCount) {
        return _readable[0]->readDirectory(path, log);
    }

    log->error(PRIME_LOCALISE("%s: No locations from which to read directories."));
    return NULL;
}

bool MultiFileSystem::getSystemPath(std::string& systemPath, const char* path, FileProperties* fileProperties)
{
    for (size_t i = 0; i != _readableCount; ++i) {
        if (getSystemPath(systemPath, path, fileProperties)) {
            return true;
        }
    }

    return false;
}
}
