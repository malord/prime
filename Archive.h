// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ARCHIVE_H
#define PRIME_ARCHIVE_H

#include "ArchiveReader.h"
#include <vector>

namespace Prime {

/// A collection of files from one or more ArchiveReaders. Implements ArchiveReader.
class PRIME_PUBLIC Archive : public ArchiveReader {
public:
    /// Identifies an individual file within an ArchiveReader and records the file's properties.
    class PRIME_PUBLIC File : public NonAtomicRefCounted<File> {
    public:
        File();

        File(ArchiveReader* reader, const DirectoryEntry& directoryEntry);

        ~File();

        ArchiveReader* getArchiveReader() const { return _reader; }

        const DirectoryEntry& getProperties() const { return _directoryEntry; }

        RefPtr<Stream> open(ArchiveReader::OpenFileOptions flags, Log* log) const
        {
            return _reader->openFile(_directoryEntry.getID(), flags, log);
        }

        bool copy(Log* sourceLog, Stream* destination, Log* destinationLog, ArchiveReader::OpenFileOptions flags,
            size_t bufferSize = 0, void* buffer = NULL)
        {
            return _reader->copyFile(_directoryEntry.getID(), sourceLog, destination, destinationLog, flags, bufferSize, buffer);
        }

    private:
        DirectoryEntry _directoryEntry;
        RefPtr<ArchiveReader> _reader;
    };

    Archive();

    ~Archive();

    /// Add all the files and archive properties from an ArchiveReader.
    bool load(ArchiveReader* archiveReader, Log* log);

    /// Add files from an ArchiveReader but don't copy any of the archive's properties.
    bool addFiles(ArchiveReader* archiveReader, Log* log);

    size_t getFileCount() const { return _files.size(); }

    const File* getFile(size_t index) const { return _files[index]; }

    void removeFile(size_t index) { _files.erase(_files.begin() + index); }

    void addFile(File* file) { _files.push_back(file); }

    void addFile(ArchiveReader* archiveReader, const DirectoryEntry& directoryEntry);

    const Value::Dictionary& getProperties() const { return _properties; }

    void setProperties(Value::Dictionary properties) { _properties.swap(properties); }

    // ArchiveReader implementation.
    virtual bool open(FileSystem* fileSystem, const char* path, OpenArchiveOptions openFlags, Log* log) PRIME_OVERRIDE;
    virtual ReadDirectoryResult readDirectoryEntry(DirectoryEntry& directoryEntry, Log* log) PRIME_OVERRIDE;
    virtual bool doFileContentsFollowDirectoryEntries() const PRIME_OVERRIDE;
    virtual RefPtr<Stream> openFile(const Value& identifier, OpenFileOptions flags, Log* log) PRIME_OVERRIDE;
    virtual bool copyFile(const Value& identifier, Log* sourceLog, Stream* destination, Log* destinationLog,
        OpenFileOptions flags, size_t bufferSize = 0, void* buffer = NULL) PRIME_OVERRIDE;
    virtual Value::Dictionary getArchiveProperties() PRIME_OVERRIDE;
    virtual bool reopen(Log* log) PRIME_OVERRIDE;

private:
    std::vector<RefPtr<File>> _files;
    Value::Dictionary _properties;

    ptrdiff_t _reading;

    PRIME_UNCOPYABLE(Archive);
};
}

#endif
