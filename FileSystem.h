// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_FILESYSTEM_H
#define PRIME_FILESYSTEM_H

#include "Optional.h"
#include "Stream.h"
#include "UnixTime.h"

namespace Prime {

/// A virtual file system - could be a zip file, an HTTP directory or the system file system. '/' should be
/// used as the path separator on all platforms (it will be converted to the native path separator by the
/// implementation).
class PRIME_PUBLIC FileSystem : public RefCounted {
    PRIME_DECLARE_UID_CAST_BASE(0x55b0c72b, 0x890a4a44, 0x9a66d249, 0x9ef38343)

public:
    FileSystem() { }

    virtual ~FileSystem();

    enum CompressionMethod {
        CompressionMethodNone,
        CompressionMethodDeflate
    };

    /// Both open() and test() can (in some implementations) acquire some properties on a file accessible to the
    /// FileSystem. All properties are optional and not all will be available on all file systems.
    /// This class can be extended and FileSystems can provide additional information.
    struct PRIME_PUBLIC FileProperties {
        PRIME_DECLARE_UID_CAST_BASE(0x7305de23, 0xd8b846f8, 0xa14719d6, 0x781b9bec)

    public:
        FileProperties()
            : isDirectory(false)
        {
        }

        virtual ~FileProperties()
        {
        }

        bool isDirectory;

        Optional<UnixTime> modificationTime;
        Optional<CompressionMethod> compressionMethod;
        Optional<Stream::Offset> size;
        Optional<uint32_t> crc32;
    };

    class OpenOptions {
        PRIME_DECLARE_UID_CAST_BASE(0xb0e6a8d0, 0x062e4477, 0x984e156f, 0x2b64163a)

    public:
        OpenOptions()
            : _doNotDecompress(false)
            , _doNotVerifyChecksum(false)
            , _writeAtomically(false)
        {
        }

        virtual ~OpenOptions()
        {
        }

        /// e.g., to send a zip'd file over HTTP using deflate/gzip.
        OpenOptions& setDoNotDecompress(bool value = true)
        {
            _doNotDecompress = value;
            return *this;
        }
        bool getDoNotDecompress() const { return _doNotDecompress; }

        /// This is implied by setDoNotDecompress, since the checksum applies to decompressed data.
        OpenOptions& setDoNotVerifyChecksum(bool value = true)
        {
            _doNotVerifyChecksum = value;
            return *this;
        }
        bool getDoNotVerifyChecksum() const { return _doNotVerifyChecksum; }

        /// Implementing this is optional (but SystemFileSystem implements it).
        OpenOptions& setWriteAtomically(bool value = true)
        {
            _writeAtomically = value;
            return *this;
        }
        bool getWriteAtomically() const { return _writeAtomically; }

        virtual bool areAnyNonIgnorableOptionsSet() const { return _doNotDecompress || _doNotVerifyChecksum; }

    private:
        bool _doNotDecompress;
        bool _doNotVerifyChecksum;
        bool _writeAtomically;
    };

    /// All FileSystems should use UNIX slashes (/).
    virtual RefPtr<Stream> open(const char* path, const OpenMode& openMode, Log* log,
        const OpenOptions& openOptions = OpenOptions(),
        FileProperties* fileProperties = NULL)
        = 0;

    /// Default implementation tries to open the file.
    virtual bool test(const char* path, FileProperties* fileProperties = NULL);

    virtual bool remove(const char* path, Log* log);

    virtual bool rename(const char* from, const char* to, Log* log, bool overwrite = false);

    class PRIME_PUBLIC DirectoryReader : public RefCounted {
        PRIME_DECLARE_UID_CAST_BASE(0xe4612ffa, 0xc46e4db7, 0x935286f3, 0x0bf20420)

    public:
        virtual ~DirectoryReader();

        /// Read the next directory entry. Returns false on error or if there are no more entries to read
        /// (use *error to distinguish).
        virtual bool read(Log* log, bool* error = NULL) = 0;

        /// Returns the file name, without path, of the directory entry.
        virtual const char* getName() const = 0;

        virtual bool isDirectory() const = 0;

        virtual bool isHidden() const;

        virtual bool isLink() const;

        /// Returns false for symlinks, sockets, etc.
        virtual bool isFile() const;
    };

    virtual RefPtr<DirectoryReader> readDirectory(const char* path, Log* log);

    /// Returns false if the file does not exist on the system file system, or if an error occurs.
    virtual bool getSystemPath(std::string& systemPath, const char* path, FileProperties* fileProperties = NULL);

    //
    // Helper methods
    //

    RefPtr<Stream> openForRead(const char* path, Log* log, FileProperties* fileProperties = NULL)
    {
        return open(path, OpenMode().setRead(), log, OpenOptions(), fileProperties);
    }

    RefPtr<Stream> openForWrite(const char* path, Log* log, FileProperties* fileProperties = NULL)
    {
        return open(path, OpenMode().setOverwrite(), log, OpenOptions(), fileProperties);
    }

    RefPtr<Stream> openForAtomicWrite(const char* path, Log* log, FileProperties* fileProperties = NULL)
    {
        return open(path, OpenMode().setOverwrite(), log, OpenOptions().setWriteAtomically(), fileProperties);
    }

private:
    PRIME_UNCOPYABLE(FileSystem);
};
}

#endif
