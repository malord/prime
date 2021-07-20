// Copyright 2000-2021 Mark H. P. Lord

#include "SystemFileSystem.h"
#include "DirectoryReader.h"
#include "File.h"
#include "FileProperties.h"
#include "FileStream.h"
#include "Path.h"
#include "PrefixLog.h"
#include "TempFile.h"

namespace Prime {

void SystemFileSystem::setPath(const char* path)
{
    _path = path;
}

#ifdef PRIME_HAVE_FILEPROPERTIES
static void TranslateFileProperties(FileSystem::FileProperties* out, const Prime::FileProperties& in)
{
    out->isDirectory = in.isDirectory();
    out->modificationTime = in.getModificationTime();
    out->size = in.getSize();
}
#endif

void SystemFileSystem::computeFullPath(std::string& fullPath, const char* path) const
{
    fullPath = Path::join(_path, Path::fixForwardSlashes(path));
}

RefPtr<Stream> SystemFileSystem::open(const char* path, const OpenMode& openMode, Log* log,
    const OpenOptions& openOptions, FileProperties* fileProperties)
{
    std::string fullPath;
    computeFullPath(fullPath, path);

    PrefixLog prefixLog(log, Format("(%s)", fullPath.c_str()));

    RefPtr<Stream> stream;
    RefPtr<FileStream> fileStream;

    if (openMode.getCreate() && openMode.getTruncate() && openMode.getWrite() && openOptions.getWriteAtomically()) {
        RefPtr<TempFile> tempFile = PassRef(new TempFile);
        if (!tempFile->createToOverwrite(fullPath.c_str(), prefixLog)) {
            return NULL;
        }

        stream = tempFile;
    } else {
        fileStream = PassRef(new FileStream);
        if (!fileStream->open(fullPath.c_str(), openMode, prefixLog)) {
            return NULL;
        }

        stream = fileStream;
    }

    if (fileProperties) {
        // Reset *fileProperties first.
        *fileProperties = FileProperties();

#ifdef PRIME_HAVE_FILEPROPERTIES
        Prime::FileProperties props;

#ifdef PRIME_HAVE_FILEPROPERTIES_STATHANDLE
        if (fileStream) {
            if (!props.readHandle(fileStream->getFileNo(), prefixLog)) {
                return NULL;
            }
        } else
#endif
        {
            if (!props.read(fullPath.c_str(), prefixLog)) {
                return NULL;
            }
        }

        TranslateFileProperties(fileProperties, props);
#endif
    }

    return stream;
}

#ifdef PRIME_HAVE_DIRECTORYREADER

namespace {

    template <typename PlatformDirectoryReader>
    class DirectoryReaderWrapper : public FileSystem::DirectoryReader {
    public:
        /// Open a directory for reading. "path" is the path to a directory and cannot contain a wildcard.
        bool open(const char* path, Log* log)
        {
            return _dir.open(path, log);
        }

        //
        // FileSystem::DirectoryReader implementation.
        //

        virtual bool read(Log* log, bool* error) PRIME_OVERRIDE
        {
            return _dir.read(log, error);
        }

        virtual const char* getName() const PRIME_OVERRIDE
        {
            return _dir.getName();
        }

        virtual bool isDirectory() const PRIME_OVERRIDE
        {
            return _dir.isDirectory();
        }

        virtual bool isHidden() const PRIME_OVERRIDE
        {
            return _dir.isHidden();
        }

        virtual bool isLink() const PRIME_OVERRIDE
        {
            return _dir.isLink();
        }

        virtual bool isFile() const PRIME_OVERRIDE
        {
            return _dir.isFile();
        }

    private:
        PlatformDirectoryReader _dir;
    };
}

#endif

RefPtr<FileSystem::DirectoryReader> SystemFileSystem::readDirectory(const char* path, Log* log)
{
#ifndef PRIME_HAVE_DIRECTORYREADER
    return FileSystem::readDirectory(path, log);
#else
    std::string fullPath;
    computeFullPath(fullPath, path);

    PrefixLog prefixLog(log, Format("(%s)", fullPath.c_str()));

    typedef DirectoryReaderWrapper<Prime::DirectoryReader> PlatformDirectoryReaderWrapper;

    RefPtr<PlatformDirectoryReaderWrapper> directoryReader = PassRef(new PlatformDirectoryReaderWrapper);
    if (!directoryReader->open(fullPath.c_str(), prefixLog)) {
        return NULL;
    }

    return directoryReader;
#endif
}

bool SystemFileSystem::remove(const char* path, Log* log)
{
    std::string fullPath;
    computeFullPath(fullPath, path);

    PrefixLog prefixLog(log, Format("(%s)", fullPath.c_str()));

    return RemoveFile(fullPath.c_str(), prefixLog);
}

bool SystemFileSystem::rename(const char* from, const char* to, Log* log, bool overwrite)
{
    std::string fullFrom;
    computeFullPath(fullFrom, from);

    std::string fullTo;
    computeFullPath(fullTo, to);

    PrefixLog prefixLog(log, Format("(%s => %s)", fullFrom.c_str(), fullTo.c_str()));

    return overwrite ? RenameFileOverwrite(fullFrom.c_str(), fullTo.c_str(), prefixLog)
                     : RenameFile(fullFrom.c_str(), fullTo.c_str(), prefixLog);
}

bool SystemFileSystem::getSystemPath(std::string& systemPath, const char* path, FileProperties* fileProperties)
{
    if (!test(path, fileProperties)) {
        return false;
    }

    computeFullPath(systemPath, path);
    return true;
}
}
