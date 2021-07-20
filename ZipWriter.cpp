// Copyright 2000-2021 Mark H. P. Lord

#include "ZipWriter.h"

#ifdef PRIME_HAVE_ZIPWRITER

#include "CRC32.h"
#include "DeflateStream.h"
#include "HashStream.h"
#include "NumberUtils.h"
#include "Substream.h"

namespace Prime {

ZipWriter::ZipWriter()
{
}

ZipWriter::~ZipWriter()
{
}

bool ZipWriter::begin(Stream* stream, Log* log, const Options& options)
{
    _copyBuffer.reset(new char[options._copyBufferSize]);

    _options = options;
    _stream = stream;
    _log = log;

    _centralDirectory.clear();
    _fileCount = 0;

    return true;
}

bool ZipWriter::beginFile(const Zip::CentralDirectoryEntry& partialCentralDirectoryEntry)
{
    _lentOffset = _stream->getOffset(_log);
    if (_lentOffset < 0) {
        return false;
    }

    Zip::LocalDirectoryEntry lent;
    lent.copyCentralDirectoryEntry(partialCentralDirectoryEntry);

    size_t lentSize = lent.computeEncodedSize();

    std::vector<char> zeros(lentSize, '\0');
    if (lentSize && !_stream->writeExact(&zeros[0], lentSize, _log)) {
        return false;
    }

    return true;
}

bool ZipWriter::endFile(const Zip::CentralDirectoryEntry& partialCentralDirectoryEntry, const char* filename, const char* extra, const char* comment)
{
    Zip::LocalDirectoryEntry lent;
    lent.copyCentralDirectoryEntry(partialCentralDirectoryEntry);
    lent.signature = Zip::LocalDirectoryEntry::validSignature;

    size_t lentSize = lent.computeEncodedSize();
    std::vector<char> lentBuffer(lentSize);

    // Encode the local directory entry.
    lent.encode(&lentBuffer[0], filename, extra);

    Stream::Offset nextFileOffset = _stream->getOffset(_log);
    if (nextFileOffset < 0) {
        return false;
    }

    // Seek back and overwrite the empty directory entry we wrote earlier.
    if (!_stream->setOffset(_lentOffset, _log)) {
        return false;
    }

    if (lentSize && !_stream->writeExact(&lentBuffer[0], lentSize, _log)) {
        return false;
    }

    // Return to the end of the archive.
    if (!_stream->setOffset(nextFileOffset, _log)) {
        return false;
    }

    Zip::CentralDirectoryEntry cent = partialCentralDirectoryEntry;
    cent.offset = (uint32_t)_lentOffset;
    cent.signature = Zip::CentralDirectoryEntry::validSignature;

    size_t centSize = cent.computeEncodedSize();
    std::vector<char> centBuffer(centSize); // TODO: just write direct to _centralDirectory?
    cent.encode(&centBuffer[0], filename, extra, comment);

    if (centSize) {
        _centralDirectory.insert(_centralDirectory.end(), &centBuffer[0], &centBuffer[0] + centSize);
    }
    ++_fileCount;

    return true;
}

bool ZipWriter::end()
{
    if (!writeCentralDirectory()) {
        return false;
    }

    if (!writeEnd()) {
        return false;
    }

    _stream.release();
    _log.release();
    _copyBuffer.reset();

    return true;
}

bool ZipWriter::writeCentralDirectory()
{
    _centralDirectoryOffset = _stream->getOffset(_log);
    if (_centralDirectoryOffset < 0) {
        return false;
    }

    return !_centralDirectory.empty() && _stream->writeExact(&_centralDirectory[0], _centralDirectory.size(), _log);
}

bool ZipWriter::writeEnd()
{
    if ((uint16_t)_fileCount != _fileCount) {
        _log->error(PRIME_LOCALISE("Zip archive's cannot contain more than 65,535 files."));
        return false;
    }

    if (_fileCount > 32767) {
        _log->warning(PRIME_LOCALISE("Zip contains more than 32,767 files, which may cause compatibility problems."));
    }

    Zip::EndRecord end;

    end.signature = Zip::EndRecord::validSignature;
    end.thisDiskNumber = 0;
    end.cdirDiskNumber = 0;
    end.cdirThisDisk = 0;
    end.cdirEntryCount = (uint16_t)_fileCount;
    end.cdirSize = (uint32_t)_centralDirectory.size();
    end.cdirOffset = (uint32_t)_centralDirectoryOffset;
    end.commentLength = 0;

    char endBuffer[Zip::EndRecord::encodedSize];
    end.encode(endBuffer);

    if (!_stream->writeExact(endBuffer, sizeof(endBuffer), _log)) {
        return false;
    }

    Stream::Offset totalFileSize = _stream->getOffset(_log);
    if (totalFileSize < 0) {
        return false;
    }

    if ((uint32_t)totalFileSize != (uint64_t)totalFileSize) {
        _log->error(PRIME_LOCALISE("Zip archive exceeds 4 gigabytes."));
        return false;
    }

    return true;
}

bool ZipWriter::compressFileAndComputeCRC32(Stream* source, uint32_t& compressedSizeOut, uint32_t& crc32Out,
    uint16_t& methodOut, CompressionCallback compressionCallback)
{
    _compressionCallback = compressionCallback;

    Stream::Offset uncompressedSize = source->getSize(_log);
    if (uncompressedSize < 0) {
        return false;
    }

    Stream::Offset sourceStartOffset = source->getOffset(_log);
    if (sourceStartOffset < 0) {
        return false;
    }

    Stream::Offset dataOffset = _stream->getOffset(_log);
    if (dataOffset < 0) {
        return false;
    }

    // source -> HashStream<CRC32> -> DeflateStream -> Substream -> archive
    // The DeflateStream is skipped if no-compression is desired. The order of these declaration is important
    // (as the destruction order is significant).
    Substream substream;
    DeflateStream deflater;
    HashStream<CRC32> crc32Stream;

    // Use a Substream to ensure the compressed file doesn't exceed the size of the uncompressed file.
    substream.init(_stream.get(), dataOffset, false, (Stream::Offset)uncompressedSize, _log);
    substream.setSilentlyDetectWriteOverflow(true);

    // Use a DeflateStream to perform the compression.
    if (_options._compressionLevel) {
        deflater.setCompressionLevel(_options._compressionLevel);
        if (!deflater.init(&substream, _log, _options._deflateBufferSize)) {
            return false;
        }
    }

    // Use a CRC32Stream to compute the CRC-32.
    if (_options._compressionLevel) {
        methodOut = Zip::CompressionMethodDeflate;
        crc32Stream.setStream(&deflater);
    } else {
        methodOut = Zip::CompressionMethodStore;
        crc32Stream.setStream(&substream);
    }

    if (!copyBytesAcrossStreams(&crc32Stream, source, uncompressedSize)) {
        return false;
    }

    if (!crc32Stream.flush(_log)) {
        return false;
    }

    if (_options._compressionLevel) {
        if (!deflater.end(_log)) {
            return false;
        }
    }

    if (!substream.flush(_log)) {
        return false;
    }

    crc32Out = crc32Stream.getHash();

    if (substream.didWriteOverflow()) {
        PRIME_ASSERT(_options._compressionLevel); // If this failed, the _uncompressed_ bytes exceed the original size of the file!

        methodOut = Zip::CompressionMethodStore;
        compressedSizeOut = (uint32_t)uncompressedSize;

        if (!_stream->setOffset(dataOffset, _log)) {
            return false;
        }

        // TODO: have an option to prevent clamping of the compressed file
        // TODO: have a callback that'll re-open the Stream for us to avoid this issue

        if (!source->setOffset(sourceStartOffset, _log)) {
            _log->developerWarning("ZipWriter: unable to rewind Stream after compression failed.");
            return false;
        }

        if (!copyBytesAcrossStreams(_stream.get(), source, uncompressedSize)) {
            return false;
        }
    } else {
        Stream::Offset newOffset = _stream->getOffset(_log);
        if (newOffset < 0) {
            return false;
        }

        compressedSizeOut = (uint32_t)(newOffset - dataOffset);
    }

    return true;
}

bool ZipWriter::copyBytesAcrossStreams(Stream* dest, Stream* source, uint64_t bytesToCopy)
{
    uint64_t remaining = bytesToCopy;

    while (remaining) {
        if (_compressionCallback) {
            _compressionCallback(bytesToCopy - remaining, bytesToCopy);
        }

        size_t maximumToTransfer = Min<size_t>((size_t)remaining, _options._copyBufferSize);

        ptrdiff_t got = source->readSome(_copyBuffer.get(), maximumToTransfer, _log);

        if (got <= 0) {
            if (got == 0) {
                _log->error(PRIME_LOCALISE("Unexpected end of source file during compression."));
            }

            return false;
        }

        if (!dest->writeExact(_copyBuffer.get(), got, _log)) {
            return false;
        }

        remaining -= got;
    }

    return true;
}
}

#endif // PRIME_HAVE_DEFLATESTREAM
