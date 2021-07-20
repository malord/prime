// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ARCHIVEFILESYSTEM_H
#define PRIME_ARCHIVEFILESYSTEM_H

#include "ArchiveReader.h"
#include "FileSystem.h"
#include <string>

namespace Prime {

/// A FileSystem that reads files from an ArchiveReader.
class PRIME_PUBLIC ArchiveFileSystem : public FileSystem {
    PRIME_DECLARE_UID_CAST(FileSystem, 0x333f9e04, 0x657b4619, 0x96c39b01, 0xd99724aa)

public:
    ArchiveFileSystem();

    virtual ~ArchiveFileSystem();

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _skipPrefix(true)
            , _ignoreChecksum(false)
            , _ignoreCase(false)
        {
        }

        /// Only provide access to files in this path within the archive.
        Options& setPrefix(std::string value)
        {
            _prefix.swap(value);
            return *this;
        }
        const std::string& getPrefix() const { return _prefix; }

        /// If there's a prefix, should it be skipped (so /assets/file.ext will become file.ext)? Default is true.
        Options& setShouldSkipPrefix(bool value)
        {
            _skipPrefix = value;
            return *this;
        }
        bool getShouldSkipPrefix() const { return _skipPrefix; }

        /// If true, don't check checksums when reading files.
        Options& setIgnoreChecksum(bool value)
        {
            _ignoreChecksum = value;
            return *this;
        }
        bool getIgnoreChecksum() const { return _ignoreChecksum; }

        /// Use case insensitive file name comparisons.
        Options& setIgnoreCase(bool value)
        {
            _ignoreCase = value;
            return *this;
        }
        bool getIgnoreCase() const { return _ignoreCase; }

    private:
        std::string _prefix;
        bool _skipPrefix;
        bool _ignoreChecksum;
        bool _ignoreCase;
    };

    /// Retains the ArchiveReader.
    bool init(ArchiveReader* archiveReader, const Options& options, Log* log);

    // FileSystems implementation.
    virtual RefPtr<Stream> open(const char* path, const OpenMode& openMode, Log* log,
        const OpenOptions& openOptions = OpenOptions(),
        FileProperties* fileProperties = NULL) PRIME_OVERRIDE;
    virtual bool test(const char* path, FileProperties* fileProperties = NULL) PRIME_OVERRIDE;
    virtual RefPtr<DirectoryReader> readDirectory(const char* path, Log* log) PRIME_OVERRIDE;

private:
    class ArchiveDirectoryReader;
    friend class ArchiveDirectoryReader;

    struct File : public NonAtomicRefCounted<File> {
        ArchiveReader::DirectoryEntry directoryEntry;
        std::string name; // Tidied path, possibly with a suffix stripped. Original name is in directoryEntry.

        class Comparator {
            bool _ignoreCase;

        public:
            explicit Comparator(bool ignoreCase)
                : _ignoreCase(ignoreCase)
            {
            }

            bool operator()(const RefPtr<File>& lhs, const RefPtr<File>& rhs) const;
        };

        class LessThanPathComparator {
            bool _ignoreCase;

        public:
            explicit LessThanPathComparator(bool ignoreCase)
                : _ignoreCase(ignoreCase)
            {
            }

            bool operator()(const RefPtr<File>& file, const char* path) const;
        };

        class EqualsPathComparator {
            bool _ignoreCase;

        public:
            explicit EqualsPathComparator(bool ignoreCase)
                : _ignoreCase(ignoreCase)
            {
            }

            bool operator()(const RefPtr<File>& file, const char* path) const;
        };
    };

    static const char* matchPrefix(const char* path, const char* internalPrefix);

    static void fixPath(std::string& fixed, const char* path, bool keepTrailingSlashes = false);

    RefPtr<File> findFile(const char* path) const;

    static void setFileProperties(FileProperties* fileProperties, const File& file);

    std::vector<RefPtr<File>> _files;

    Options _options;

    RefPtr<ArchiveReader> _archiveReader;

    PRIME_UNCOPYABLE(ArchiveFileSystem);
};

}

#endif
