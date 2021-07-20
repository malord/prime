// Copyright 2000-2021 Mark H. P. Lord

#include "TempDirectory.h"
#include "File.h"
#include "Path.h"

namespace Prime {

TempDirectory::TempDirectory()
{
}

TempDirectory::~TempDirectory()
{
    close();
}

void TempDirectory::close()
{
    if (_removeOnDestructLog.get()) {
        remove(_removeOnDestructLog);
        _removeOnDestructLog.release();
    }
}

bool TempDirectory::createInPath(const char* path, bool removeOnDestruct, Log* log, unsigned int permissions)
{
    if (_removeOnDestructLog.get()) {
        remove(_removeOnDestructLog);
    }

    _removeOnDestructLog = removeOnDestruct ? log : NULL;

    std::string pathTemplate = Path::join(path, "temp_XXXXXXXX");

    std::string filename; // Hoisted

    const int maxAttempts = 100;
    for (int attempt = 0; attempt != maxAttempts; ++attempt) {
        filename = pathTemplate;
        filename.c_str();

        if (!MakeTempName(&filename[0])) {
            continue;
        }

        if (FileExists(filename.c_str(), log)) {
            continue;
        }

        if (!MakePath(filename.c_str(), log, permissions)) {
            continue;
        }

        _path.swap(filename);
        return true;
    }

    return false;
}

bool TempDirectory::remove(Log* log)
{
    if (_path.empty()) {
        return true;
    }

    bool result = RecursiveRemove(_path.c_str(), log);

    _path.resize(0);
    return result;
}
}
