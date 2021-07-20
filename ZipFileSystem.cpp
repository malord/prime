// Copyright 2000-2021 Mark H. P. Lord

#include "ZipFileSystem.h"

#ifdef PRIME_HAVE_ZIPFILESYSTEM

#include "FileProperties.h"
#include "Path.h"
#include "PrefixLog.h"
#include "SystemFileSystem.h"
#include "Templates.h"

namespace Prime {

//
// ZipFileSystem::ZipDirectoryReader
//

class ZipFileSystem::ZipDirectoryReader : public FileSystem::DirectoryReader {
public:
    ZipDirectoryReader(ZipFileSystem* zipFileSystem, const char* path);

    void putBack() { --_index; }

    // FileSystem::DirectoryReader implementation.
    virtual bool read(Log* log, bool* error = NULL) PRIME_OVERRIDE;
    virtual const char* getName() const PRIME_OVERRIDE;
    virtual bool isDirectory() const PRIME_OVERRIDE;
    virtual bool isHidden() const PRIME_OVERRIDE;

private:
    RefPtr<ZipFileSystem> _zipFileSystem;
    std::string _path;
    ptrdiff_t _index;
};

ZipFileSystem::ZipDirectoryReader::ZipDirectoryReader(ZipFileSystem* zipFileSystem, const char* path)
{
    _zipFileSystem = zipFileSystem;

    _path = path;

    _index = -1;
}

bool ZipFileSystem::ZipDirectoryReader::read(Log* log, bool* error)
{
    // TODO: need to figure out a way of reading directories whose entries aren't in the zip file

    (void)log;

    if (error) {
        *error = false;
    }

    while (++_index != (ptrdiff_t)_zipFileSystem->_files.size()) {
        PRIME_ASSERT(_index >= 0);
        const File& file = *_zipFileSystem->_files[_index];

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

const char* ZipFileSystem::ZipDirectoryReader::getName() const
{
    return _zipFileSystem->_files[_index]->name.c_str() + _path.size();
}

bool ZipFileSystem::ZipDirectoryReader::isDirectory() const
{
    return _zipFileSystem->_files[_index]->isDirectory;
}

bool ZipFileSystem::ZipDirectoryReader::isHidden() const
{
    return false;
}

//
// ZipFileSystem comparators
//

bool ZipFileSystem::File::Comparator::operator()(const RefPtr<const File>& lhs, const RefPtr<const File>& rhs) const
{
    return (_ignoreCase ? ASCIICompareIgnoringCase(lhs->name.c_str(), rhs->name.c_str()) : strcmp(lhs->name.c_str(), rhs->name.c_str())) < 0;
}

bool ZipFileSystem::File::LessThanPathComparator::operator()(const RefPtr<const File>& file, const char* path) const
{
    return (_ignoreCase ? ASCIICompareIgnoringCase(file->name.c_str(), path) : strcmp(file->name.c_str(), path)) < 0;
}

bool ZipFileSystem::File::EqualsPathComparator::operator()(const RefPtr<const File>& file, const char* path) const
{
    return (_ignoreCase ? ASCIICompareIgnoringCase(file->name.c_str(), path) : strcmp(file->name.c_str(), path)) == 0;
}

//
// ZipFileSystem
//

PRIME_DEFINE_UID_CAST(ZipFileSystem)

RefPtr<FileSystem> ZipFileSystem::createFileSystemForZipOrDirectory(const char* path, Log* log)
{
    PrefixLog pathLog(log, path);

    Prime::FileProperties fileProperties;

    if (!fileProperties.read(path, pathLog) || fileProperties.isDirectory()) {
        log->trace("Mounting directory: %s", path);
        RefPtr<SystemFileSystem> staticFileSystem = PassRef(new SystemFileSystem);
        staticFileSystem->setPath(path);
        return staticFileSystem;
    }

#ifdef PRIME_HAVE_ZIPFILESYSTEM

    RefPtr<ZipFileSystem> zipFileSystem = PassRef(new ZipFileSystem);
    ZipFileSystem::Options zipOptions;
    log->trace("Mounting zip file: %s", path);
    if (!zipFileSystem->init(PassRef(new SystemFileSystem), path, zipOptions, pathLog)) {
        return NULL;
    }

    return zipFileSystem;

#else

    log->error(PRIME_LOCALISE("zlib disabled"));

#endif
}

ZipFileSystem::ZipFileSystem()
{
}

ZipFileSystem::~ZipFileSystem()
{
}

bool ZipFileSystem::init(FileSystem* zipFileSystem, const char* zipFilePath, const Options& options, Log* log)
{
    if (!_zipReader.open(zipFileSystem, zipFilePath, ZipReader::Options(), log)) {
        return false;
    }

    std::string fixedPath;

    ZipReader::ReadDirectoryResult readDirectoryResult;
    while ((readDirectoryResult = _zipReader.readDirectoryEntry(log)) == ZipReader::ReadDirectoryResultOK) {
        const char* name = _zipReader.getFilename().c_str();
        fixPath(fixedPath, name);

        if (options.getPrefix()) {
            name = matchPrefix(fixedPath.c_str(), options.getPrefix());
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

        file->token = _zipReader.getFileToken();
        file->modificationDate = _zipReader.getFileModificationDate();
        file->modificationTime = _zipReader.getFileModificationTime();
        file->isDirectory = _zipReader.isDirectory();
        file->name.swap(fixedPath);

        _files.push_back(RefPtr<const File>(file));
    }

    if (readDirectoryResult != ZipReader::ReadDirectoryResultEnd) {
        return false;
    }

    File::Comparator comparator(options.getIgnoreCase());
    std::sort(_files.begin(), _files.end(), comparator);

    _options = options;

    return true;
}

const char* ZipFileSystem::matchPrefix(const char* path, const char* internalPrefix)
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

    if (GenericPath::stripLeadingSlashesView(internalPrefix).empty()) {
        return path;
    }

    return NULL;
}

RefPtr<const ZipFileSystem::File> ZipFileSystem::findFile(const char* path) const
{
    std::string fixedPath;
    fixPath(fixedPath, path);

    File::LessThanPathComparator fileLessThanPathComparator(_options.getIgnoreCase());
    File::EqualsPathComparator fileEqualsPathComparator(_options.getIgnoreCase());

    std::vector<RefPtr<const File>>::const_iterator iter = LowerBound(_files.begin(), _files.end(), fixedPath.c_str(), fileLessThanPathComparator);
    if (iter == _files.end() || !fileEqualsPathComparator(*iter, fixedPath.c_str())) {
        return NULL;
    }

    return *iter;
}

bool ZipFileSystem::test(const char* path, FileProperties* fileProperties)
{
    RefPtr<const File> file = findFile(path);
    if (!file) {
        return false;
    }

    setFileProperties(fileProperties, *file);
    return true;
}

void ZipFileSystem::setFileProperties(FileProperties* fileProperties, const File& file)
{
    if (fileProperties) {
        *fileProperties = FileProperties();
        fileProperties->isDirectory = file.isDirectory;
        fileProperties->size = file.token.decompressedSize;
        fileProperties->crc32 = file.token.crc32;
        fileProperties->modificationTime = ZipReader::zipDateTimeToUnixTime(file.modificationDate, file.modificationTime);
        switch (file.token.method) {
        case Zip::CompressionMethodDeflate:
            fileProperties->compressionMethod = CompressionMethodDeflate;
            break;
        case Zip::CompressionMethodStore:
            fileProperties->compressionMethod = CompressionMethodNone;
            break;
        }
    }
}

RefPtr<Stream> ZipFileSystem::open(const char* path, const OpenMode& openMode, Log* log,
    const OpenOptions& openOptions, FileProperties* fileProperties)
{
    if (openMode.isWriteAccessRequired()) {
        log->error(PRIME_LOCALISE("Can't write to a zip file."));
        return NULL;
    }

    RefPtr<const File> file = findFile(path);
    if (!file) {
        log->error(PRIME_LOCALISE("File not found."));
        return NULL;
    }

    if (file->isDirectory) {
        log->error(PRIME_LOCALISE("Cannot open directory as a stream."));
        return NULL;
    }

    setFileProperties(fileProperties, *file);

    ZipReader::StreamOptions zipReaderOptions;
    if (_options.getIgnoreCRC() || openOptions.getDoNotVerifyChecksum()) {
        zipReaderOptions.setDoNotVerifyCRC();
    }
    if (openOptions.getDoNotDecompress()) {
        zipReaderOptions.setDoNotDecompress();
    }

    return _zipReader.openFile(file->token, zipReaderOptions, log);
}

void ZipFileSystem::fixPath(std::string& fixed, const char* path, bool keepTrailingSlashes)
{
    path = GenericPath::skipSlashes(path);
    fixed = GenericPath::tidy(path, keepTrailingSlashes ? GenericPath::FixPathKeepTrailingSlashes : 0);
}

RefPtr<FileSystem::DirectoryReader> ZipFileSystem::readDirectory(const char* path, Log* log)
{
    std::string fixedPath;
    fixPath(fixedPath, path);

    if (!fixedPath.empty()) {
        fixedPath += '/';
    }

    RefPtr<ZipDirectoryReader> directoryReader = PassRef(new ZipDirectoryReader(this, fixedPath.c_str()));

    if (!directoryReader->read(Log::getNullLog())) {
        log->error(PRIME_LOCALISE("Path not found."));
        return NULL;
    }

    directoryReader->putBack();

    return directoryReader;
}

}

#endif // PRIME_HAVE_ZIPFILESYSTEM
