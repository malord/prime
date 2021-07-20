// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ARCHIVEREADER_H
#define PRIME_ARCHIVEREADER_H

#include "Config.h"
#include "FileSystem.h"
#include "Log.h"
#include "Value.h"

namespace Prime {

/// An object capable of enumerating and reading the files within an archive.
class PRIME_PUBLIC ArchiveReader : public RefCounted {
public:
    // Keys returned in the dictionaries (both the archive properties dictionary and a DirectoryEntry's properties
    // dictionary).
    static const char modificationTimePropertyName[];
    static const char crc32PropertyName[];
    static const char compressionMethodPropertyName[];
    static const char commentPropertyName[];

    class PRIME_PUBLIC DirectoryEntry {
    public:
        typedef uint64_t Size;

        DirectoryEntry()
            : _isDirectory(false)
            , _unpackedSize(0)
            , _packedSize(0)
        {
        }

        /// Returns an ID that can be used later to uniquely identify this file within this archive. The ID is
        /// invalidated if another archive is opened or the archive is rewound. IDs are not guaranteed to be
        /// consecutive (they may be physical file offsets or memory addresses for example).
        const Value& getID() const { return _id; }

        void setID(Value identifier) { _id.move(identifier); }

        /// Return the name of the fetched directory entry. Paths should be normalised to use UNIX (/) path
        /// separators and must not begin with a /, nor contain a drive letter.
        const std::string& getName() const { return _name; }

        void setName(std::string name) { _name.swap(name); }

        /// Returns true if the fetched directory entry is a directory.
        bool isDirectory() const { return _isDirectory; }

        void setDirectory(bool directory) { _isDirectory = directory; }

        Size getUnpackedSize() const { return _unpackedSize; }

        /// Returns the packed size of the file. -1 if not known.
        void setUnpackedSize(Size size) { _unpackedSize = size; }

        /// Returns the unpacked size of the file. -1 if not known. Will be equal to getFilePackedSize() for
        /// uncompressed files.
        Size getPackedSize() const { return _packedSize; }

        void setPackedSize(Size size) { _packedSize = size; }

        /// Set both the packed and unpacked size (for non-compressed files).
        void setSize(Size size) { _packedSize = _unpackedSize = size; }

        const Value& getProperty(const char* key) const { return _dictionary[key]; }

        bool hasProperty(const char* key) const { return _dictionary.has(key); }

        void setProperty(const char* key, Value value) { _dictionary.set(key, PRIME_MOVE(value)); }

        const Value::Dictionary& getProperties() const { return _dictionary; }

        void setProperties(Value::Dictionary dict) { _dictionary.swap(dict); }

    private:
        std::string _name;
        Value _id;
        bool _isDirectory;
        Size _unpackedSize;
        Size _packedSize;
        Value::Dictionary _dictionary;
    };

    virtual ~ArchiveReader() { }

    /// Flags for open()'s openFlags parameter.
    class PRIME_PUBLIC OpenArchiveOptions {
    public:
    };

    /// Open an archive for reading. You can then call readDirectoryEntry() to fetch the first directory entry.
    /// You don't pass a Stream because we could need to fetch additional volumes from a multi-part archive.
    virtual bool open(FileSystem* fileSystem, const char* path, OpenArchiveOptions openFlags, Log* log) = 0;

    /// Return values for readDirectoryEntry().
    enum ReadDirectoryResult {
        /// An error was found in the archive. An error message will have been logged.
        ReadDirectoryResultError,

        /// A directory entry has been read. You can now query it with getFilename(), etc.
        ReadDirectoryResultOK,

        /// The end of the directory has been reached.
        ReadDirectoryResultEnd
    };

    /// Fetch the next directory entry from the archive.
    virtual ReadDirectoryResult readDirectoryEntry(DirectoryEntry& directoryEntry, Log* log) = 0;

    /// Returns true if directory entries are immediately followed by that file's data. If so, the caller can
    /// avoid seeking by reading the file in its entirety immediately after fetching the directory entry.
    virtual bool doFileContentsFollowDirectoryEntries() const = 0;

    /// Flags for openFile() and related methods.
    class OpenFileOptions {
    public:
        OpenFileOptions()
            : _decompress(true)
            , _verifyChecksum(true)
        {
        }

        /// Defaults to true. Use this to transfer an already compressed file without recompression.
        /// Implies verify checksum is disabled.
        OpenFileOptions& setDecompress(bool value)
        {
            _decompress = value;
            return *this;
        }
        bool getDecompress() const { return _decompress; }

        /// Whether or not to verify the checksum of the data. Performance optimisation.
        OpenFileOptions& setVerifyChecksum(bool value)
        {
            _verifyChecksum = value;
            return *this;
        }
        bool getVerifyChecksum() const { return _verifyChecksum; }

    private:
        bool _decompress;
        bool _verifyChecksum;
    };

    /// Open a file for reading, given it's ID (as returned by DirectoryEntry::getID()). This can be called any
    /// time after the directory entry corresponding to the file ID has been read.
    virtual RefPtr<Stream> openFile(const Value& identifier, OpenFileOptions flags, Log* log) = 0;

    /// Read a file and copy it to the supplied stream. Not all decompressors can return a Stream that can
    /// efficiently read from compressed data. For such streams, this will be faster.
    virtual bool copyFile(const Value& identifier, Log* sourceLog, Stream* destination, Log* destinationLog,
        OpenFileOptions flags, size_t bufferSize = 0, void* buffer = NULL);

    /// Returns a dictionary containing the archive's properties. A different dictionary may be returned as more
    /// directory entries are read, and a complete dictionary is not guaranteed to be available until the end of
    /// the archive has been reached (i.e., until readDirectoryEntry() has returned ReadDirectoryResultEnd).
    virtual Value::Dictionary getArchiveProperties() = 0;

    /// Reopen the archive, as though open() had just been called. This invalidates all the file IDs.
    virtual bool reopen(Log* log) = 0;
};

}

#endif
