// Copyright 2000-2021 Mark H. P. Lord

#include "ZipArchiveReader.h"

#ifdef PRIME_HAVE_ZIPREADER

#include "DateTime.h"

namespace Prime {

const char ZipArchiveReader::zipExternalAttributesPropertyName[] = "zipExternalAttributes";
const char ZipArchiveReader::zipInternalAttributesPropertyName[] = "zipInternalAttributes";
const char ZipArchiveReader::zipExtraDataPropertyName[] = "zipExtraData";

ZipArchiveReader::ZipArchiveReader()
{
}

ZipArchiveReader::~ZipArchiveReader()
{
}

bool ZipArchiveReader::open(FileSystem* fileSystem, const char* path, OpenArchiveOptions openFlags, Log* log)
{
    (void)openFlags;
    return _zipReader.open(fileSystem, path, _options, log);
}

bool ZipArchiveReader::reopen(Log* log)
{
    return _zipReader.reopen(log);
}

bool ZipArchiveReader::doFileContentsFollowDirectoryEntries() const
{
    return true;
}

ArchiveReader::ReadDirectoryResult ZipArchiveReader::readDirectoryEntry(DirectoryEntry& directoryEntry, Log* log)
{
    ZipReader::ReadDirectoryResult zipReaderResult = _zipReader.readDirectoryEntry(log);
    switch (zipReaderResult) {
    default:
        PRIME_ASSERT(0);
    case ZipReader::ReadDirectoryResultError:
        return ReadDirectoryResultError;

    case ZipReader::ReadDirectoryResultEnd:
        return ReadDirectoryResultEnd;

    case ZipReader::ReadDirectoryResultOK:
        break;
    }

    directoryEntry = DirectoryEntry();
    directoryEntry.setName(_zipReader.getFilename());
    ZipReader::Token identifier = _zipReader.getFileToken();
    directoryEntry.setID(Data(&identifier, sizeof(identifier)));
    directoryEntry.setDirectory(_zipReader.isDirectory());
    directoryEntry.setPackedSize(_zipReader.getFilePackedSize());
    directoryEntry.setUnpackedSize(_zipReader.getFileUnpackedSize());

    directoryEntry.setProperty(modificationTimePropertyName, ZipReader::zipDateTimeToUnixTime(_zipReader.getFileModificationDate(), _zipReader.getFileModificationTime()));
    directoryEntry.setProperty(crc32PropertyName, Value::Integer(_zipReader.getFileCRC32()));
    directoryEntry.setProperty(zipExternalAttributesPropertyName, Value::Integer(_zipReader.getFileExternalAttributes()));
    directoryEntry.setProperty(zipInternalAttributesPropertyName, Value::Integer(_zipReader.getFileInternalAttributes()));

    switch (_zipReader.getFileCompressionMethod()) {
    case Zip::CompressionMethodStore:
        // No compressionMethod property for uncompressed files
        break;

    case Zip::CompressionMethodDeflate: {
        directoryEntry.setProperty(compressionMethodPropertyName, "Deflate");
        break;
    }

    default:
        directoryEntry.setProperty(compressionMethodPropertyName, "Unknown");
    }

    size_t extraDataSize;
    const uint8_t* extra = (const uint8_t*)_zipReader.getFileExtraData(extraDataSize);

    if (extra && extraDataSize) {
        directoryEntry.setProperty(zipExtraDataPropertyName, Data(extra, extraDataSize));
    }

    const std::string& comment = _zipReader.getFileComment();

    if (!comment.empty()) {
        directoryEntry.setProperty(commentPropertyName, comment);
    }

    return ReadDirectoryResultOK;
}

RefPtr<Stream> ZipArchiveReader::openFile(const Value& identifier, OpenFileOptions flags, Log* log)
{
    ZipReader::StreamOptions options;

    if (!flags.getDecompress()) {
        options.setDoNotDecompress();
    }

    if (!flags.getVerifyChecksum()) {
        options.setDoNotVerifyCRC();
    }

    const ZipReader::Token* zipToken = (const ZipReader::Token*)identifier.getData().data();

    return _zipReader.openFile(*zipToken, options, log);
}

Value::Dictionary ZipArchiveReader::getArchiveProperties()
{
    Value::Dictionary dict;

    dict.set(commentPropertyName, _zipReader.getArchiveComment());

    return dict;
}
}

#endif // PRIME_HAVE_ZIPREADER
