// Copyright 2000-2021 Mark H. P. Lord

#include "FileLoader.h"
#include "FileStream.h"
#include "PrefixLog.h"
#ifdef PRIME_FILELOADER_SUPPORT_STDIN
#include "StdioStream.h"
#endif

namespace Prime {

bool FileLoader::load(const char* path, Log* log)
{
    PrefixLog prefixLog(log, path);

    FileStream file;
    if (!file.openForRead(path, prefixLog)) {
        return false;
    }

    return _loader.load(&file, prefixLog);
}

#ifdef PRIME_FILELOADER_SUPPORT_STDIN

bool FileLoader::loadSupportingStdin(const char* path, Log* log)
{
    if (strcmp(path, "-") != 0) {
        return load(path, log);
    }

    PrefixLog prefixLog(log, "<stdin>");

    StdioStream stdio(stdin, false);
    return _loader.load(&stdio, prefixLog);
}

#endif
}
