// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ZIPFILESYSTEM_H
#define PRIME_ZIPFILESYSTEM_H

#include "ZipReader.h"

#ifdef PRIME_HAVE_ZIPREADER

#define PRIME_HAVE_ZIPFILESYSTEM

#include "FileSystem.h"
#include <string>
#include <vector>

#define PRIME_HAVE_ZIPFILESYSTEM

namespace Prime {

/// A FileSystem that reads files from a zip file. (You could also use ArchiveFileSystem with a ZipArchiveReader,
/// but this has slightly more efficient storage of directory entries.)
class PRIME_PUBLIC ZipFileSystem : public FileSystem {
    PRIME_DECLARE_UID_CAST(FileSystem, 0x333f9e04, 0x657b4619, 0x96c39b01, 0xd99724aa)

public:
    /// If path refers to a directory, returns a SystemFileSystem to access its contents. Otherwise, the file is
    /// assumed to be a zip file and a ZipFileSystem is returned.
    static RefPtr<FileSystem> createFileSystemForZipOrDirectory(const char* path, Log* log);

    ZipFileSystem();

    virtual ~ZipFileSystem();

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _prefix(NULL)
            , _skipPrefix(true)
            , _ignoreCRC(false)
            , _ignoreCase(true)
        {
        }

        /// Only provide access to files in this path within the zip.
        Options& setPrefix(const char* value)
        {
            _prefix = value;
            return *this;
        }
        const char* getPrefix() const { return _prefix; }

        /// If there's a prefix, should it be skipped (so /assets/file.ext will become file.ext)? Default is true.
        Options& setShouldSkipPrefix(bool value)
        {
            _skipPrefix = value;
            return *this;
        }
        bool getShouldSkipPrefix() const { return _skipPrefix; }

        /// If true, don't check the CRC-32 when reading files.
        Options& setIgnoreCRC(bool value)
        {
            _ignoreCRC = value;
            return *this;
        }
        bool getIgnoreCRC() const { return _ignoreCRC; }

        /// Ignore case in file names (default is true).
        Options& setIgnoreCase(bool value)
        {
            _ignoreCase = value;
            return *this;
        }
        bool getIgnoreCase() const { return _ignoreCase; }

    private:
        const char* _prefix;
        bool _skipPrefix;
        bool _ignoreCRC;
        bool _ignoreCase;
    };

    /// Reads the zip directory and retains a reference to the FileSystem so it can be reopened as necessary.
    bool init(FileSystem* zipFileSystem, const char* zipFilePath, const Options& options, Log* log);

    // FileSystems implementation.
    virtual RefPtr<Stream> open(const char* path, const OpenMode& openMode, Log* log,
        const OpenOptions& openOptions = OpenOptions(),
        FileProperties* fileProperties = NULL) PRIME_OVERRIDE;
    virtual bool test(const char* path, FileProperties* fileProperties = NULL) PRIME_OVERRIDE;
    virtual RefPtr<DirectoryReader> readDirectory(const char* path, Log* log) PRIME_OVERRIDE;

private:
    class ZipDirectoryReader;
    friend class ZipDirectoryReader;

    ZipReader _zipReader;

    /// Non-atomic because the files list isn't changed once the directory has been read.
    struct File : public NonAtomicRefCounted<File> {
        ZipReader::Token token;
        std::string name;
        bool isDirectory;
        uint16_t modificationDate, modificationTime;

        class Comparator {
            bool _ignoreCase;

        public:
            explicit Comparator(bool ignoreCase)
                : _ignoreCase(ignoreCase)
            {
            }

            bool operator()(const RefPtr<const File>& lhs, const RefPtr<const File>& rhs) const;
        };

        class LessThanPathComparator {
            bool _ignoreCase;

        public:
            explicit LessThanPathComparator(bool ignoreCase)
                : _ignoreCase(ignoreCase)
            {
            }

            bool operator()(const RefPtr<const File>& file, const char* path) const;
        };

        class EqualsPathComparator {
            bool _ignoreCase;

        public:
            explicit EqualsPathComparator(bool ignoreCase)
                : _ignoreCase(ignoreCase)
            {
            }

            bool operator()(const RefPtr<const File>& file, const char* path) const;
        };
    };

    static const char* matchPrefix(const char* path, const char* internalPrefix);

    static void fixPath(std::string& fixed, const char* path, bool keepTrailingSlashes = false);

    RefPtr<const File> findFile(const char* path) const;

    static void setFileProperties(FileProperties* fileProperties, const File& file);

    std::vector<RefPtr<const File>> _files;

    Options _options;

    PRIME_UNCOPYABLE(ZipFileSystem);
};

}

#endif // PRIME_HAVE_ZIPREADER

#endif
