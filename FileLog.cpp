// Copyright 2000-2021 Mark H. P. Lord

#include "FileLog.h"
#include "Convert.h"
#include "File.h"
#include "FileStream.h"
#include "LogRecorder.h"
#include "Path.h"
#include <string.h>

namespace Prime {

FileLog::FileLog()
{
    setTimePrefix(true);
}

FileLog::~FileLog()
{
}

bool FileLog::init(const char* path, Log* log, const Options& options)
{
    _options = options;
    _path = path;
    _log.setLog(log);
    _log.setPrefix(path);

    if (!_mutex.init(log, "FileLog mutex")) {
        return false;
    }

    OpenMode openMode;
    openMode.setWrite().setAppend().setCreate().setTruncate(_options.getTruncate());

    FileStream stream;
    if (!stream.open(_path.c_str(), openMode, _log)) {
        return false;
    }

    return true;
}

void FileLog::write(Level level, const char* string)
{
    (void)level;

    if (_path.empty()) {
        return;
    }

    RecursiveMutex::ScopedLock lock(&_mutex);

    FileStream stream;

    if (!stream.open(_path.c_str(), OpenMode().setWrite().setAppend().setCreate(), _log)) {
        return;
    }

    stream.writeExact(string, strlen(string), _log);

    if (stream.getOffset(_log) >= _options.getMaxFileSize()) {
        stream.close(_log);

        LogRecorder recorder;
        PrefixLog recorderPrefix(&recorder, "Archiving logs");

        for (int n = _options.getMaxFiles() - 1; n > 0; --n) {
            std::string pathMinus1 = getPathForArchive(n - 1);
            std::string path = getPathForArchive(n);

            if (FileExists(pathMinus1.c_str(), Log::getNullLog())) {
                RenameFileOverwrite(pathMinus1.c_str(), path.c_str(), &recorderPrefix);
            }
        }

        if (recorder.getMaxLevel() != LevelNone) {
            recorder.replay(this);
        }
    }
}

std::string FileLog::getPathForArchive(int n) const
{
    if (n == 0) {
        return _path;
    }

    return MakeString(Path::stripExtensionView(_path), '.', n, Path::exetensionView(_path));
}

bool FileLog::clearLogs(Log* log)
{
    RecursiveMutex::ScopedLock lock(&_mutex);

    bool removedAll = true;

    for (int i = 0; i < _options.getMaxFiles(); i++) {
        std::string path = getPathForArchive(i);
        if (FileExists(path.c_str(), Log::getNullLog()) && !RemoveFile(path.c_str(), log)) {
            removedAll = false;
        }
    }

    return removedAll;
}
}
