// Copyright 2000-2021 Mark H. P. Lord

#include "ArchiveFileSystem.h"
#include "Path.h"
#include "Templates.h"

namespace Prime {

//
// ArchiveFileSystem::ArchiveDirectoryReader
//

class ArchiveFileSystem::ArchiveDirectoryReader : public FileSystem::DirectoryReader {
public:
    ArchiveDirectoryReader(ArchiveFileSystem* archiveFileSystem, const char* path);

    void putBack() { --_index; }

    // FileSystem::DirectoryReader implementation.
    virtual bool read(Log* log, bool* error = NULL) PRIME_OVERRIDE;
    virtual const char* getName() const PRIME_OVERRIDE;
    virtual bool isDirectory() const PRIME_OVERRIDE;
    virtual bool isHidden() const PRIME_OVERRIDE;

private:
    RefPtr<ArchiveFileSystem> _archiveFileSystem;
    std::string _path;
    ptrdiff_t _index;
};

ArchiveFileSystem::ArchiveDirectoryReader::ArchiveDirectoryReader(ArchiveFileSystem* archiveFileSystem, const char* path)
{
    _archiveFileSystem = archiveFileSystem;

    _path = path;

    _index = -1;
}

bool ArchiveFileSystem::ArchiveDirectoryReader::read(Log* log, bool* error)
{
    // TODO: need to figure out a way of reading directories whose entries aren't in the zip file

    if (error) {
        *error = false;
    }

    (void)log;
    while (++_index != (ptrdiff_t)_archiveFileSystem->_files.size()) {
        PRIME_ASSERT(_index >= 0);
        const File& file = *_archiveFileSystem->_files[_index];

        if (ASCIICompareIgnoringCase(_path.data(), file.name.c_str(), _path.size()) != 0) {
            continue;
        }

        if (strchr(file.name.c_str() + _path.size(), '/')) {
            continue;
        }

        return true;
    }

    --_index;
    return false;
}

const char* ArchiveFileSystem::ArchiveDirectoryReader::getName() const
{
    return _archiveFileSystem->_files[_index]->name.c_str() + _path.size();
}

bool ArchiveFileSystem::ArchiveDirectoryReader::isDirectory() const
{
    return _archiveFileSystem->_files[_index]->directoryEntry.isDirectory();
}

bool ArchiveFileSystem::ArchiveDirectoryReader::isHidden() const
{
    return false;
}

//
// ArchiveFileSystem comparators
//

bool ArchiveFileSystem::File::Comparator::operator()(const RefPtr<File>& lhs, const RefPtr<File>& rhs) const
{
    return (_ignoreCase ? ASCIICompareIgnoringCase(lhs->name.c_str(), rhs->name.c_str()) : strcmp(lhs->name.c_str(), rhs->name.c_str())) < 0;
}

bool ArchiveFileSystem::File::LessThanPathComparator::operator()(const RefPtr<File>& file, const char* path) const
{
    return (_ignoreCase ? ASCIICompareIgnoringCase(file->name.c_str(), path) : strcmp(file->name.c_str(), path)) < 0;
}

bool ArchiveFileSystem::File::EqualsPathComparator::operator()(const RefPtr<File>& file, const char* path) const
{
    return (_ignoreCase ? ASCIICompareIgnoringCase(file->name.c_str(), path) : strcmp(file->name.c_str(), path)) == 0;
}

//
// ArchiveFileSystem
//

PRIME_DEFINE_UID_CAST(ArchiveFileSystem)

ArchiveFileSystem::ArchiveFileSystem()
{
}

ArchiveFileSystem::~ArchiveFileSystem()
{
}

bool ArchiveFileSystem::init(ArchiveReader* archiveReader, const Options& options, Log* log)
{
    _archiveReader = archiveReader;

    std::string fixedPath;

    ArchiveReader::ReadDirectoryResult readDirectoryResult;
    ArchiveReader::DirectoryEntry directoryEntry;
    while ((readDirectoryResult = _archiveReader->readDirectoryEntry(directoryEntry, log)) == ArchiveReader::ReadDirectoryResultOK) {
        const char* name = directoryEntry.getName().c_str();
        fixPath(fixedPath, name);

        if (!options.getPrefix().empty()) {
            name = matchPrefix(fixedPath.c_str(), options.getPrefix().c_str());
            if (!name) {
                continue;
            }

            if (options.getShouldSkipPrefix()) {
                if (name != fixedPath.data()) {
                    name = GenericPath::skipSlashes(name);
                    fixedPath.erase(fixedPath.begin(), fixedPath.begin() + (name - fixedPath.data()));
                }
            }
        }

        RefPtr<File> file = PassRef(new File);

        file->directoryEntry = directoryEntry;
        file->directoryEntry.setName(""); // Don't need the original name
        file->name.swap(fixedPath);

        _files.push_back(file);
    }

    if (readDirectoryResult != ArchiveReader::ReadDirectoryResultEnd) {
        return false;
    }

    File::Comparator comparator(options.getIgnoreCase());
    std::sort(_files.begin(), _files.end(), comparator);

    _options = options;

    return true;
}

const char* ArchiveFileSystem::matchPrefix(const char* path, const char* internalPrefix)
{
    if (!internalPrefix) {
        return path;
    }

    path = GenericPath::skipSlashes(path);
    internalPrefix = GenericPath::skipSlashes(internalPrefix);

    while (ASCIIToLower(*path) == ASCIIToLower(*internalPrefix) && *path) {
        if (*path == '/') {
            path = GenericPath::skipSlashes(path);
            internalPrefix = GenericPath::skipSlashes(internalPrefix);
        } else {
            ++path;
            ++internalPrefix;
        }
    }

    if (!*GenericPath::skipSlashes(internalPrefix)) {
        return path;
    }

    return NULL;
}

RefPtr<ArchiveFileSystem::File> ArchiveFileSystem::findFile(const char* path) const
{
    std::string fixedPath;
    fixPath(fixedPath, path);

    File::LessThanPathComparator fileLessThanPathComparator(_options.getIgnoreCase());
    File::EqualsPathComparator fileEqualsPathComparator(_options.getIgnoreCase());

    std::vector<RefPtr<File>>::const_iterator iter = LowerBound(_files.begin(), _files.end(), fixedPath.c_str(), fileLessThanPathComparator);
    if (iter == _files.end() || !fileEqualsPathComparator(*iter, fixedPath.c_str())) {
        return NULL;
    }

    return *iter;
}

bool ArchiveFileSystem::test(const char* path, FileProperties* fileProperties)
{
    RefPtr<File> file = findFile(path);
    if (!file) {
        return false;
    }

    setFileProperties(fileProperties, *file);
    return true;
}

void ArchiveFileSystem::setFileProperties(FileProperties* fileProperties, const File& file)
{
    if (fileProperties) {
        *fileProperties = FileProperties();
        fileProperties->isDirectory = file.directoryEntry.isDirectory();
        fileProperties->size = file.directoryEntry.getUnpackedSize();

        Value crc32Value = file.directoryEntry.getProperty(ArchiveReader::crc32PropertyName);
        if (!crc32Value.isUndefined()) {
            fileProperties->crc32 = (uint32_t)crc32Value.toInteger();
        }

        Value methodValue = file.directoryEntry.getProperty(ArchiveReader::compressionMethodPropertyName);
        if (!methodValue.isUndefined()) {
            if (methodValue.getString() == "Deflate") {
                fileProperties->compressionMethod = CompressionMethodDeflate;
            }
        }
    }
}

RefPtr<Stream> ArchiveFileSystem::open(const char* path, const OpenMode& openMode,
    Log* log, const OpenOptions& openOptions, FileProperties* fileProperties)
{
    if (openMode.isWriteAccessRequired()) {
        log->error(PRIME_LOCALISE("Can't write to an archive."));
        return NULL;
    }

    RefPtr<File> file = findFile(path);
    if (!file) {
        log->error(PRIME_LOCALISE("File not found."));
        return NULL;
    }

    if (file->directoryEntry.isDirectory()) {
        log->error(PRIME_LOCALISE("Cannot open directory as a stream."));
        return NULL;
    }

    setFileProperties(fileProperties, *file);

    ArchiveReader::OpenFileOptions fileStreamOptions;
    if (_options.getIgnoreChecksum() || openOptions.getDoNotVerifyChecksum()) {
        fileStreamOptions.setVerifyChecksum(false);
    }
    if (openOptions.getDoNotDecompress()) {
        fileStreamOptions.setDecompress(false);
    }

    return _archiveReader->openFile(file->directoryEntry.getID(), fileStreamOptions, log);
}

void ArchiveFileSystem::fixPath(std::string& fixed, const char* path, bool keepTrailingSlashes)
{
    path = GenericPath::skipSlashes(path);
    fixed = GenericPath::tidy(path, keepTrailingSlashes ? GenericPath::FixPathKeepTrailingSlashes : 0);
}

RefPtr<FileSystem::DirectoryReader> ArchiveFileSystem::readDirectory(const char* path, Log* log)
{
    std::string fixedPath;
    fixPath(fixedPath, path);

    if (!fixedPath.empty()) {
        fixedPath += '/';
    }

    RefPtr<ArchiveDirectoryReader> directoryReader = PassRef(new ArchiveDirectoryReader(this, fixedPath.c_str()));

    if (!directoryReader->read(Log::getNullLog())) {
        log->error(PRIME_LOCALISE("Path not found."));
        return NULL;
    }

    directoryReader->putBack();

    return directoryReader;
}

}
