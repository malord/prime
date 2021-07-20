// Copyright 2000-2021 Mark H. P. Lord

// Sequential reading is not yet complete as there's no way to read a file from the archive without reopening it.

#include "ZipReader.h"

#ifdef PRIME_HAVE_ZIPREADER

#include "ByteOrder.h"
#include "CRC32.h"
#include "Clocks.h"
#include "HashStream.h"
#include "InflateStream.h"
#include "NumberUtils.h"
#include "Substream.h"
#include <string.h>

namespace Prime {

ZipReader::ZipReader()
{
}

ZipReader::~ZipReader()
{
}

bool ZipReader::open(FileSystem* fileSystem, const char* path, const Options& options, Log* log)
{
    _fileSystem = fileSystem;
    _archivePath = path;
    _options = options;

    return reopen(log);
}

bool ZipReader::reopen(Log* log)
{
    _stream = _fileSystem->openForRead(_archivePath.c_str(), log);

    if (!_stream) {
        return false;
    }

    _atEnd = false;
    _nextEnt = 0;
    _end.signature = 0; // Mark the end record as invalid.
    _triedSequential = false;

    if (!_stream->isSeekable() && tryBeginSequentialRead(log)) {
        return true;
    }

    // It's not a local directory entry. Find the central directory instead.
    return findCentralDirectory(log);
}

bool ZipReader::tryBeginSequentialRead(Log* log)
{
    if (_triedSequential || _stream->getOffset(log) != 0) {
        return false;
    }

    _triedSequential = true;

    if (readLocalDirectoryEntry(true, log)) {
        log->trace(PRIME_LOCALISE("Zip file opened for sequential read."));
        _fetched = true;
        _sequential = true;
        return true;
    }

    return false;
}

bool ZipReader::findCentralDirectory(Log* log)
{
    int64_t zipSize = _stream->getSize(Log::getNullLog());
    if (zipSize < 0) {
        // If we can't determine the zip file's size then we may not be able to seek, so try the sequential
        // method.
        if (tryBeginSequentialRead(log)) {
            return true;
        }

        log->error(PRIME_LOCALISE("Can't determine size of zip file (file may not be seekable)."));
        return false;
    }

    int64_t offset = zipSize - Zip::EndRecord::encodedSize;
    if (offset < 0) {
        log->error(PRIME_LOCALISE("File is too small to be a zip file."));
        return false;
    }

    char buffer[256];

    for (;;) {
        int thisTime = Min<int>((int)(zipSize - offset), (int)COUNTOF(buffer));

        if (!_stream->setOffset(offset, Log::getNullLog())) {
            // If we can't seek then try sequential method.
            if (tryBeginSequentialRead(log)) {
                return true;
            }

            log->error(PRIME_LOCALISE("Unable to seek to zip end record."));
            return false;
        }

        if (!_stream->readExact(buffer, thisTime, log)) {
            log->error(PRIME_LOCALISE("Unable to read zip end record."));
            return false;
        }

        const char* ptr = buffer;
        const char* end = buffer + thisTime - (Zip::EndRecord::encodedSize - 1);
        for (; ptr != end; ptr++) {
            if (ptr[0] == '\x50' && ptr[1] == '\x4b' && ptr[2] == '\x05' && ptr[3] == '\x06') {
                int64_t endOffset = offset + (ptr - buffer);
                unsigned int commentSize = Read16LE(ptr + 20);

                if (endOffset + Zip::EndRecord::encodedSize + commentSize == zipSize) {
                    return readEndRecord(endOffset, log);
                }
            }
        }

        if (offset == 0) {
            log->error(PRIME_LOCALISE("Central directory end record not found; probably not a valid zip file."));
            return false;
        }

        offset -= (sizeof(buffer) - Zip::EndRecord::encodedSize);

        if (offset < 0) {
            offset = 0;
        }
    }
}

bool ZipReader::readEndRecord(int64_t endOffset, Log* log)
{
    char end[Zip::EndRecord::encodedSize];

    if (!_stream->setOffset(endOffset, log)) {
        log->error(PRIME_LOCALISE("Unable to seek to zip end record."));
        return false;
    }

    if (!_stream->readExact(end, Zip::EndRecord::encodedSize, log)) {
        log->error(PRIME_LOCALISE("Unable to read zip end record."));
        return false;
    }

    if (!_end.decode(end)) {
        log->error(PRIME_LOCALISE("Invalid zip end record; file may not be a zip file."));
        return false;
    }

    readString(_zipComment, _end.commentLength, log);

    _sequential = false;
    _nextEnt = endOffset - _end.cdirSize;
    _zipOffset = _nextEnt - _end.cdirOffset;
    if (_zipOffset != 0) {
        log->trace(PRIME_LOCALISE("Zip file has %" PRIME_PRId_STREAM " excess bytes at the beginning (ignoring)."), _zipOffset);
    }

    if (!_stream->setOffset(_nextEnt, log)) {
        log->error(PRIME_LOCALISE("Unable to seek to first central directory entry."));
        return false;
    }

    _fetched = false;

    return true;
}

ZipReader::ReadDirectoryResult ZipReader::readDirectoryEntry(Log* log)
{
    if (_atEnd) {
        return ReadDirectoryResultEnd;
    }

    if (_fetched) {
        _fetched = false;
        return ReadDirectoryResultOK;
    }

    bool success = _sequential ? readLocalDirectoryEntry(false, log) : readCentralDirectoryEntry(log);

    if (!success) {
        return ReadDirectoryResultError;
    }

    _entToken.offset = _ent.offset;
    _entToken.crc32 = _ent.crc32;
    _entToken.compressedSize = _ent.compressedSize;
    _entToken.decompressedSize = _ent.decompressedSize;
    _entToken.method = _ent.method;

    if (_atEnd) {
        return ReadDirectoryResultEnd;
    }

    return ReadDirectoryResultOK;
}

bool ZipReader::readLocalDirectoryEntry(bool exploratory, Log* log)
{
    if (!_stream->setOffset(_nextEnt, log)) {
        log->error(PRIME_LOCALISE("Seek to next local directory entry failed."));

        return false;
    }

    char buffer[Zip::LocalDirectoryEntry::encodedSize];
    ptrdiff_t got = _stream->read(buffer, Zip::LocalDirectoryEntry::encodedSize, log);

    if (got < 0) {
        log->error(PRIME_LOCALISE("Read error reading zip local directory entry."));
        return false;
    }

    if (got >= 4 && Read32LE(buffer) == Zip::CentralDirectoryEntry::validSignature) {
        // We got a cental directory entry. Skip to the end of the zip file.
        skipCentralDirectory(buffer, got, log);

        reachedEnd();
        return true;
    }

    if (got >= 4 && Read32LE(buffer) == Zip::EndRecord::validSignature) {
        // We got the end record.
        skipEndRecord(buffer, got, log);

        reachedEnd();
        return true;
    }

    if (got != Zip::LocalDirectoryEntry::encodedSize) {
        log->error(PRIME_LOCALISE("Unexpected end of file within local directory entry."));
        return false;
    }

    Zip::LocalDirectoryEntry lent;
    if (!lent.decode(buffer)) {
        if (!exploratory) {
            log->error(PRIME_LOCALISE("Expected zip local directory entry but got incorrect signature."));
        }

        return false;
    }

    if (lent.bitFlag & 8) {
        if (!exploratory) {
            log->error(PRIME_LOCALISE("Data descriptors not supported."));
        }

        return false;
    }

    _ent.copyLocalDirectoryEntry(lent);
    readString(_filename, lent.filenameLength, log);
    normaliseFilename(_filename);
    readString(_fileExtraData, lent.extraLength, log);
    _ent.offset = (uint32_t)_stream->getOffset(log);
    _nextEnt = _stream->getOffset(log) + _ent.compressedSize;
    return true;
}

namespace {

    inline bool IsDriveLetter(char ch)
    {
        if (ch >= 'a' && ch <= 'z') {
            return true;
        }

        if (ch >= 'A' && ch <= 'Z') {
            return true;
        }

        return false;
    }
}

void ZipReader::normaliseFilename(std::string& filename)
{
    std::replace(filename.begin(), filename.end(), '\\', '/');

    std::string::iterator ptr = filename.begin();
    std::string::iterator end = filename.end();

    // Remove drive letters.
    if (filename.size() >= 2 && ptr[1] == ':' && IsDriveLetter(ptr[0])) {
        ptr += 2;
    }

    // Remove leading slashes and colons.
    while (ptr != end && (*ptr == '/' || *ptr == ':')) {
        ++ptr;
    }

    filename.erase(filename.begin(), ptr);
}

void ZipReader::skipCentralDirectory(const char* excess, size_t excessSize, Log* log)
{
    for (;;) {
        char buffer[Zip::CentralDirectoryEntry::encodedSize];
        PRIME_ASSERT(sizeof(buffer) >= excessSize);
        memcpy(buffer, excess, excessSize);
        ptrdiff_t got = _stream->read(buffer + excessSize, Zip::CentralDirectoryEntry::encodedSize - excessSize, log);
        got += excessSize;
        excessSize = 0;

        if (got < 0) {
            log->error(PRIME_LOCALISE("Read error in zip central directory."));
            return;
        }

        if (got == Zip::EndRecord::encodedSize && Read32LE(buffer) == Zip::EndRecord::validSignature) {
            skipEndRecord(buffer, got, log);
            return;
        }

        if (got < Zip::CentralDirectoryEntry::encodedSize) {
            log->error(PRIME_LOCALISE("Unexpected end of file in zip central directory (%d)."), (int)got);
            return;
        }

        if (!_ent.decode(buffer)) {
            log->error(PRIME_LOCALISE("Invalid entry in zip central directory."));
            return;
        }

        readString(_filename, _ent.filenameLength, log);
        readString(_fileExtraData, _ent.extraLength, log);
        readString(_fileComment, _ent.commentLength, log);
    }
}

void ZipReader::skipEndRecord(const char* excess, size_t excessSize, Log* log)
{
    char buffer[Zip::EndRecord::encodedSize];
    PRIME_ASSERT(sizeof(buffer) >= excessSize);
    memcpy(buffer, excess, excessSize);
    ptrdiff_t got = _stream->read(buffer + excessSize, Zip::CentralDirectoryEntry::encodedSize - excessSize, log);
    got += excessSize;
    //excessSize = 0;

    if (got != Zip::EndRecord::encodedSize) {
        log->error(PRIME_LOCALISE("Unexpected end of file in zip end record (%d)."), (int)got);
        return;
    }

    if (!_end.decode(buffer)) {
        log->error(PRIME_LOCALISE("Invalid zip end record."));
        return;
    }

    readString(_zipComment, _end.commentLength, log);
}

void ZipReader::reachedEnd()
{
    _atEnd = true;
    _stream.release();
}

bool ZipReader::readCentralDirectoryEntry(Log* log)
{
    if (!_stream->setOffset(_nextEnt, log)) {
        log->error(PRIME_LOCALISE("Seek to next central directory entry failed."));

        return false;
    }

    char buffer[Zip::CentralDirectoryEntry::encodedSize];
    ptrdiff_t got = _stream->read(buffer, Zip::CentralDirectoryEntry::encodedSize, log);

    if (got < 0) {
        log->error(PRIME_LOCALISE("Read error reading zip central directory entry."));
        return false;
    }

    if (got >= 4 && Read32LE(buffer) == Zip::EndRecord::validSignature) {
        // We've reached the end of the directory.
        reachedEnd();
        return true;
    }

    if (got != Zip::CentralDirectoryEntry::encodedSize) {
        log->error(PRIME_LOCALISE("Unexpected end of file within zip central directory entry."));
        return false;
    }

    if (!_ent.decode(buffer)) {
        log->error(PRIME_LOCALISE("Expected zip local directory entry but got incorrect signature."));
        return false;
    }

    readString(_filename, _ent.filenameLength, log);
    normaliseFilename(_filename);
    readString(_fileExtraData, _ent.extraLength, log);
    readString(_fileComment, _ent.commentLength, log);
    _nextEnt = _stream->getOffset(log);
    return true;
}

ZipReader::Token ZipReader::getFileToken()
{
    return _entToken;
}

RefPtr<Stream> ZipReader::openFile(const Token& token, const StreamOptions& options, Log* log)
{
    if (_sequential) {
        return streamForRegion(_stream, _zipOffset + token.offset, token.compressedSize,
            token.crc32, token.method, token.decompressedSize, options, log);
    }

    RefPtr<Stream> archiveStream(_fileSystem->openForRead(_archivePath.c_str(), log));
    if (!archiveStream) {
        return NULL;
    }

    // If we're not in sequential mode, we must read the local directory entry.

    // TODO: adjust offsets based on the SFX

    if (!archiveStream->setOffset(_zipOffset + token.offset, log)) {
        return NULL;
    }

    char buffer[Zip::LocalDirectoryEntry::encodedSize];
    if (!archiveStream->readExact(buffer, Zip::LocalDirectoryEntry::encodedSize, log)) {
        log->error(PRIME_LOCALISE("Couldn't read local directory entry of archived file."));
        return NULL;
    }

    Zip::LocalDirectoryEntry lent;
    if (!lent.decode(buffer)) {
        log->error(PRIME_LOCALISE("Invalid local directory entry in zip file."));
        return NULL;
    }

    Stream::Offset dataOffset = _zipOffset + token.offset + Zip::LocalDirectoryEntry::encodedSize + lent.filenameLength + lent.extraLength;

    return streamForRegion(archiveStream, dataOffset, token.compressedSize, token.crc32, token.method,
        token.decompressedSize, options, log);
}

RefPtr<Stream> ZipReader::streamForRegion(Stream* archiveStream, Stream::Offset where, Stream::Offset size,
    uint32_t crc32, int compressionMethod, Stream::Offset decompressedSize,
    const StreamOptions& options, Log* log)
{
    // Use a Substream to limit access to the relevant portion of the zip file.
    RefPtr<Substream> substream = PassRef(new Substream);
    if (!substream || !substream->init(archiveStream, where, true, size, log)) {
        return NULL;
    }

    if (options.getDoNotDecompress()) {
        return substream;
    }

    // Create a decompressor stream.
    RefPtr<Stream> decompressor;
    if (compressionMethod == Zip::CompressionMethodDeflate) {
        RefPtr<InflateStream> inflater = PassRef(new InflateStream);
        inflater->setSizeKnown(decompressedSize);

        if (!inflater->init(substream.get(), log)) {
            return NULL;
        }

        decompressor = inflater;
    } else if (compressionMethod == Zip::CompressionMethodStore) {
        // If the file is stored, just use the Substream directly.
        decompressor = substream;
    } else {
        log->error(PRIME_LOCALISE("Unsupported zip compression method."));
        return NULL;
    }

    // Wrap the decompressor in a CRC32 stream to validate the checksum.
    if (options.getDoNotVerifyCRC()) {
        return decompressor;
    }

    RefPtr<HashStream<CRC32>> crcStream = PassRef(new HashStream<CRC32>(decompressor.get()));
    crcStream->beginVerification(crc32, decompressedSize);

    return crcStream;
}

bool ZipReader::readString(std::string& string, size_t length, Log* log)
{
    string.resize(length);
    if (!_stream->readExact(&string[0], length, log)) {
        return false;
    }

    return true;
}

UnixTime ZipReader::zipDateTimeToUnixTime(uint16_t zipDate, uint16_t zipTime)
{
    int year, month, day, hour, minute, second;
    Zip::DecodeDateTime(zipDate, zipTime, year, month, day, hour, minute, second);

    return Clock::localDateTimeToUnixTime(DateTime(year, month, day, hour, minute, second));
}
}

#if 0
// Example:

using namespace Prime;

int main()
{
    SystemFileSystem fileSystem;

    ZipReader zipReader;
    if (! zipReader.open(&fileSystem, "/path/to/test.zip", 0, Log::getGlobal())) {
        ExitError();
    }

    for (;;) {
        ZipReader::ReadDirectoryResult result = zipReader.readDirectoryEntry(Log::getGlobal());

        if (result == ZipReader::ReadDirectoryResultError) {
            ExitError();
        }

        if (result == ZipReader::ReadDirectoryResultEnd) {
            break;
        }

        printf("%-40s %9" PRIME_PRId_STREAM"\n", zipReader.getFilename(), zipReader.getFileUnpackedSize());

        RefPtr<Stream> stream = zipReader.openFile(zipReader.getFileToken(), 0, Log::getGlobal());

        if (! stream) {
            ExitError("Couldn't get a zip file stream.");
        }

        int64_t totalRead = 0;

        for (;;) {
            char buffer[PRIME_BIG_STACK_BUFFER_SIZE];
            ptrdiff_t bytesRead = stream->readSome(buffer, sizeof(buffer), Log::getGlobal());
            if (bytesRead < 0) {
                ExitError();
            }

            if (bytesRead == 0) {
                break;
            }

            totalRead += bytesRead;
        }

        if (totalRead != zipReader.getFileUnpackedSize()) {
            ExitError("%s: Didn't read the entire file!", zipReader.getFilename());
        }
    }

    LogNote("All's well.");

    return 0;
}

#endif

#endif
