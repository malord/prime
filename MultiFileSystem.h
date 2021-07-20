// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MULTIFILESYSTEM_H
#define PRIME_MULTIFILESYSTEM_H

#include "FileSystem.h"

namespace Prime {

/// A FileSystem that tries to open Streams by querying a series of FileSystems (so you can construct a
/// search path for resources, for example). All requests to open a Stream with write/create flags go to a
/// single FileSystem set with setWritableFileSystem (which may also be in the readable FileSystems lists).
class PRIME_PUBLIC MultiFileSystem : public FileSystem {
public:
    MultiFileSystem()
        : _readableCount(0)
    {
    }

    /// Clear all the added FileSystems and un-set the writable FileSystem.
    void reset();

    /// Add a FileSystem. FileSystems are tried in the order they're added.
    void addFileSystem(FileSystem* fileSystem);

    /// Set the FileSystem to use for writing files. If you want this FileSystem to also be used for reading,
    /// you must also add it with addFileSystem().
    void setWritableFileSystem(FileSystem* fileSystem);

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
    RefPtr<FileSystem> _writable;

    enum { size = 6 };

    // If you add more than size FileSystems, then the size+1'th will be another MultiFileSystem
    // thereby creating a linked list of MultiFileSystems, allowing as many FileSystems as are needed.
    RefPtr<FileSystem> _readable[size + 1];
    size_t _readableCount;

    PRIME_UNCOPYABLE(MultiFileSystem);
};
}

#endif
