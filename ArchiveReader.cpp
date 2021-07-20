// Copyright 2000-2021 Mark H. P. Lord

#include "ArchiveReader.h"
#include "ScopedPtr.h"

namespace Prime {

const char ArchiveReader::modificationTimePropertyName[] = "modificationTime";
const char ArchiveReader::crc32PropertyName[] = "crc32";
const char ArchiveReader::compressionMethodPropertyName[] = "compressionMethod";
const char ArchiveReader::commentPropertyName[] = "comment";

bool ArchiveReader::copyFile(const Value& identifier, Log* sourceLog, Stream* destination, Log* destinationLog,
    OpenFileOptions flags, size_t bufferSize, void* buffer)
{
    RefPtr<Stream> stream = openFile(identifier, flags, sourceLog);
    if (!stream) {
        return false;
    }

    return destination->copyFrom(stream, sourceLog, -1, destinationLog, bufferSize, buffer);
}
}
