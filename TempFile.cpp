// Copyright 2000-2021 Mark H. P. Lord

#include "TempFile.h"
#include "File.h"
#include "Path.h"
#include "ScopedPtr.h"

#ifdef PRIME_HAVE_TEMPFILE

namespace Prime {

TempFile::TempFile()
    : _removeOnDestruct(true)
{
}

TempFile::~TempFile()
{
    if (_fileStream.isOpen() && _removeOnDestruct) {
        closeAndRemove(Log::getGlobal());
    }
}

bool TempFile::createInPath(const char* path, Log* log, unsigned int permissions)
{
    std::string pathTemplate = Path::join(path, "temp-XXXXXXXX");

    return createWithPathTemplate(pathTemplate.c_str(), log, permissions);
}

bool TempFile::createWithPathTemplate(const char* pathTemplate, Log* log, unsigned int permissions)
{
    close(log);

    std::string filename;

    const int maxAttempts = 100;
    for (int attempts = 0; attempts != maxAttempts; ++attempts) {
        filename = pathTemplate;
        filename.c_str();

        if (!MakeTempName(&filename[0])) {
            continue;
        }

        OpenMode openMode;
        openMode.setUseUnixPermissions(permissions != 0)
            .setUnixPermissions(permissions)
            .setReadWrite()
            .setCreate()
            .setDoNotOverwrite();

        Log* useLog = (attempts == maxAttempts - 1) ? log : Log::getNullLog();

        if (_fileStream.open(filename.c_str(), openMode, useLog)) {
            _path.assign(filename.c_str());
            return true;
        }
    }

    return false;
}

bool TempFile::createToOverwrite(const char* path, Log* log, unsigned int permissions)
{
    // So if you're writing MyFile.dat, you'll get MyFile.dat.XXXXXX.
    std::string buffer = path;
    buffer.append(".XXXXXX");

    if (!createWithPathTemplate(buffer.c_str(), log, permissions)) {
        return false;
    }

    setRenameOnClose(path);
    return true;
}

void TempFile::setRenameOnClose(const char* renameTo)
{
    _renameTo = renameTo;
}

void TempFile::cancelRenameOnClose()
{
    _renameTo.resize(0);
}

bool TempFile::close(Log* log)
{
    if (_renameTo.empty()) {
        return closeOrRemove(log);
    } else {
        return closeAndRename(_renameTo.c_str(), log);
    }
}

bool TempFile::closeOrRemove(Log* log)
{
    if (!_fileStream.isOpen() || _fileStream.close(log)) {
        return true;
    }

    RemoveFile(_path.c_str(), log);
    return false;
}

bool TempFile::closeAndRemove(Log* log)
{
    if (!_fileStream.isOpen()) {
        return true;
    }

    _fileStream.close(Log::getNullLog());
    return RemoveFile(_path.c_str(), log);
}

bool TempFile::closeAndRename(const char* to, Log* log)
{
    PRIME_ASSERT(!_path.empty());

    if (!closeOrRemove(log)) {
        return false;
    }

    return RenameFileOverwrite(_path.c_str(), to, log);
}

//
// TempFile Stream implementation
//

ptrdiff_t TempFile::readSome(void* buffer, size_t maxBytes, Log* log)
{
    return _fileStream.readSome(buffer, maxBytes, log);
}

ptrdiff_t TempFile::writeSome(const void* bytes, size_t maxBytes, Log* log)
{
    return _fileStream.writeSome(bytes, maxBytes, log);
}

Stream::Offset TempFile::seek(Offset offset, SeekMode mode, Log* log)
{
    return _fileStream.seek(offset, mode, log);
}

Stream::Offset TempFile::getSize(Log* log)
{
    return _fileStream.getSize(log);
}

bool TempFile::setSize(Offset newSize, Log* log)
{
    return _fileStream.setSize(newSize, log);
}

bool TempFile::flush(Log* log)
{
    return _fileStream.flush(log);
}

bool TempFile::copyFrom(Stream* source, Log* sourceLog, Offset length, Log* destLog, size_t bufferSize, void* buffer)
{
    return _fileStream.copyFrom(source, sourceLog, length, destLog, bufferSize, buffer);
}
}

#endif // PRIME_HAVE_TEMPFILE
