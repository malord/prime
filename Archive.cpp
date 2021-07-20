// Copyright 2000-2021 Mark H. P. Lord

#include "Archive.h"

namespace Prime {

//
// Archive::File
//

Archive::File::File()
{
}

Archive::File::File(ArchiveReader* reader, const DirectoryEntry& directoryEntry)
    : _reader(reader)
    , _directoryEntry(directoryEntry)
{
}

Archive::File::~File()
{
}

//
// Archive
//

Archive::Archive()
{
    _reading = -1;
}

Archive::~Archive()
{
}

bool Archive::load(ArchiveReader* archiveReader, Log* log)
{
    if (!addFiles(archiveReader, log)) {
        return false;
    }

    _properties = archiveReader->getArchiveProperties();

    return true;
}

bool Archive::addFiles(ArchiveReader* archiveReader, Log* log)
{
    for (;;) {
        DirectoryEntry directoryEntry;
        ArchiveReader::ReadDirectoryResult result = archiveReader->readDirectoryEntry(directoryEntry, log);
        if (result == ArchiveReader::ReadDirectoryResultError) {
            return false;
        }

        if (result == ArchiveReader::ReadDirectoryResultEnd) {
            break;
        }

        PRIME_ASSERT(result == ArchiveReader::ReadDirectoryResultOK);

        addFile(archiveReader, directoryEntry);
    }

    return true;
}

void Archive::addFile(ArchiveReader* archiveReader, const DirectoryEntry& directoryEntry)
{
    addFile(PassRef(new File(archiveReader, directoryEntry)));
}

bool Archive::open(FileSystem*, const char*, OpenArchiveOptions, Log*)
{
    PRIME_ASSERTMSG(0, "You can't open an Archive instance - use reopen() to restart the directory.");
    return false;
}

Archive::ReadDirectoryResult Archive::readDirectoryEntry(DirectoryEntry& directoryEntry, Log*)
{
    if (_reading >= (ptrdiff_t)_files.size()) {
        _reading = -1;
        return ReadDirectoryResultEnd;
    }

    ++_reading;

    directoryEntry = _files[_reading]->getProperties();
    directoryEntry.setID((Value::Integer)_reading);

    return ReadDirectoryResultOK;
}

bool Archive::doFileContentsFollowDirectoryEntries() const
{
    return false;
}

RefPtr<Stream> Archive::openFile(const Value& identifier, ArchiveReader::OpenFileOptions flags, Log* log)
{
    int fileID = identifier.toInt(-1);
    PRIME_ASSERT(fileID >= 0 && fileID < (int)_files.size());
    return _files[(size_t)fileID]->open(flags, log);
}

bool Archive::copyFile(const Value& identifier, Log* sourceLog, Stream* destination, Log* destinationLog,
    ArchiveReader::OpenFileOptions flags, size_t bufferSize, void* buffer)
{
    int fileID = identifier.toInt(-1);
    PRIME_ASSERT(fileID >= 0 && fileID < (int)_files.size());
    return _files[(size_t)fileID]->copy(sourceLog, destination, destinationLog, flags, bufferSize, buffer);
}

Value::Dictionary Archive::getArchiveProperties()
{
    return _properties;
}

bool Archive::reopen(Log*)
{
    _reading = -1;
    return true;
}
}
