// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ZIPREADER_H
#define PRIME_ZIPREADER_H

#include "ZipFormat.h"

#ifndef PRIME_NO_ZLIB

#include "FileSystem.h"

#define PRIME_HAVE_ZIPREADER

namespace Prime {

/// Only supports deflate-32 or uncompressed files and does not support multi-part archives.
/// Supports non-seekable Streams (e.g., you can read a zip file from a SocketStream).
class PRIME_PUBLIC ZipReader {
public:
    static UnixTime zipDateTimeToUnixTime(uint16_t zipDate, uint16_t zipTime);

    ZipReader();

    ~ZipReader();

    class PRIME_PUBLIC Options {
    public:
        Options()
        {
        }
    };

    /// A FileSystem is used so that multi-part archives could one day be supported.
    bool open(FileSystem* fileSystem, const char* path, const Options& options, Log* log);

    /// Rewinds to the beginning of the archive. Invalidates all the file indexes.
    bool reopen(Log* log);

    /// Return values for readDirectoryEntry().
    enum ReadDirectoryResult {
        /// An error was found in the archive.
        ReadDirectoryResultError,

        /// A directory entry has been read. You can now query it with getFilename(), etc.
        ReadDirectoryResultOK,

        /// The end of the directory has been reached.
        ReadDirectoryResultEnd
    };

    ReadDirectoryResult readDirectoryEntry(Log* log);

    /// The zip comment is not read until after the last directory entry has been read.
    const std::string& getArchiveComment() const { return _zipComment; }

    const std::string& getFilename() { return _filename; }

    bool isDirectory()
    {
        bool endsWithSlash = !_filename.empty() && _filename[_filename.size() - 1] == '/';

        if ((_ent.madeByVersion & 0xf0) == 0) {
            return (_ent.externalAttributes & Zip::FileAttributeDirectory) != 0 || endsWithSlash;
        }

        return endsWithSlash;
    }

    /// A token used to open a file from within the zip file.
    struct Token {
        uint32_t offset;
        uint32_t crc32;
        uint32_t compressedSize;
        uint32_t decompressedSize;
        uint16_t method;
    };

    Token getFileToken();

    int64_t getFilePackedSize() const { return _ent.compressedSize; }

    int64_t getFileUnpackedSize() const { return _ent.decompressedSize; }

    uint16_t getFileModificationDate() const { return _ent.modificationDate; }

    uint16_t getFileModificationTime() const { return _ent.modificationTime; }

    uint32_t getFileCRC32() const { return _ent.crc32; }

    unsigned int getFileExternalAttributes() const { return _ent.externalAttributes; }

    unsigned int getFileInternalAttributes() const { return _ent.internalAttributes; }

    Zip::CompressionMethod getFileCompressionMethod() const { return (Zip::CompressionMethod)_ent.method; }

    /// Never returns a null pointer.
    const std::string& getFileComment() const { return _fileComment; }

    /// Always sets *size.
    const void* getFileExtraData(size_t& size) const
    {
        size = _fileExtraData.size();
        return _fileExtraData.data();
    }

    class StreamOptions {
    public:
        StreamOptions()
            : _doNotDecompress(false)
            , _doNotVerifyCRC(false)
        {
        }

        StreamOptions& setDoNotDecompress(bool value = true)
        {
            _doNotDecompress = value;
            return *this;
        }
        bool getDoNotDecompress() const { return _doNotDecompress; }

        /// This is implied by setDoNotDecompress, since the checksum applies to decompressed data.
        StreamOptions& setDoNotVerifyCRC(bool value = true)
        {
            _doNotVerifyCRC = value;
            return *this;
        }
        bool getDoNotVerifyCRC() const { return _doNotVerifyCRC; }

    private:
        bool _doNotDecompress;
        bool _doNotVerifyCRC;
    };

    RefPtr<Stream> openFile(const Token& token, const StreamOptions& options, Log* log);

private:
    /// Read a string from the stream in to a string.
    bool readString(std::string& string, size_t length, Log* log);

    /// Read the next local directory entry from the stream. If the end of the local directory entries is
    /// found, returns true and sets _atEnd to true. Returns false if what was read is not a local directory
    /// entry.
    bool readLocalDirectoryEntry(bool exploratory, Log* log);

    /// Search the stream for the central directory.
    bool findCentralDirectory(Log* log);

    /// Read the zip end record at the specified file offset and set up for reading central directory entries.
    bool readEndRecord(int64_t endOffset, Log* log);

    /// Read the next central directory entry from the stream. If the end of the central directory is found,
    /// returns true and sets _atEnd to true. Returns false if a central directory entry was not read.
    bool readCentralDirectoryEntry(Log* log);

    /// Called by the local directory entry reader when the central directory (i.e., the end of the local
    /// directory entries) is found. Skips past the cental directory entries and reads the zip file end
    /// marker and zip comment, therefore validating the file is a valid zip file.
    void skipCentralDirectory(const char* excess, size_t excessSize, Log* log);

    /// Skip the central directory end record, reading the comment.
    void skipEndRecord(const char* excess, size_t excessSize, Log* log);

    /// Try to begin reading the archive sequentially.
    bool tryBeginSequentialRead(Log* log);

    /// Returns an AddRefd Stream that reads/decompresses data from within the archive. archiveStream should be
    /// a newly opened Stream for reading the archive.
    RefPtr<Stream> streamForRegion(Stream* archiveStream, Stream::Offset where, Stream::Offset size,
        uint32_t crc32, int compressionMethod, Stream::Offset decompressedSize,
        const StreamOptions& options, Log* log);

    /// Normalise a file name so that it uses only UNIX slashes and does not begin with a path separator or drive letter.
    void normaliseFilename(std::string& filename);

    /// Sets _atEnd to true, closes the Stream and frees the buffer memory.
    void reachedEnd();

    RefPtr<FileSystem> _fileSystem;
    std::string _archivePath;
    Options _options;

    RefPtr<Stream> _stream;

    Stream::Offset _zipOffset;

    bool _atEnd;

    uint64_t _nextEnt;

    /// True if we're reading local directory entries.
    bool _sequential;

    bool _fetched;

    /// Current central directory entry.
    Zip::CentralDirectoryEntry _ent;
    Token _entToken;

    /// Set to true if we've tried opening the archive sequentially.
    bool _triedSequential;

    std::string _filename;
    std::string _fileComment;
    std::string _fileExtraData;
    std::string _zipComment;
    Zip::EndRecord _end;

    PRIME_UNCOPYABLE(ZipReader);
};
}

#endif // PRIME_NO_ZLIB

#endif
