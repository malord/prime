// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ZIPARCHIVEREADER_H
#define PRIME_ZIPARCHIVEREADER_H

#include "ArchiveReader.h"
#include "ZipReader.h"

#ifdef PRIME_HAVE_ZIPREADER

namespace Prime {

/// An ArchiveReader implementation for zip files (actually a wrapper around ZipReader).
class PRIME_PUBLIC ZipArchiveReader : public ArchiveReader {
public:
    // Keys returned in the dictionaries.
    static const char zipExternalAttributesPropertyName[];
    static const char zipInternalAttributesPropertyName[];
    static const char zipExtraDataPropertyName[];

    ZipArchiveReader();

    ~ZipArchiveReader();

    ZipReader::Options& getOptions() { return _options; }

    // ArchiveReader implementation
    virtual bool open(FileSystem* fileSystem, const char* path, OpenArchiveOptions openFlags, Log* log) PRIME_OVERRIDE;
    virtual bool reopen(Log* log) PRIME_OVERRIDE;
    virtual bool doFileContentsFollowDirectoryEntries() const PRIME_OVERRIDE;
    virtual ReadDirectoryResult readDirectoryEntry(DirectoryEntry& directoryEntry, Log* log) PRIME_OVERRIDE;
    virtual RefPtr<Stream> openFile(const Value& identifier, OpenFileOptions flags, Log* log) PRIME_OVERRIDE;
    virtual Value::Dictionary getArchiveProperties() PRIME_OVERRIDE;

private:
    ZipReader _zipReader;
    ZipReader::Options _options;
};
}

#endif // PRIME_HAVE_ZIPREADER

#endif
