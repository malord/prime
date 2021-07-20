// Copyright 2000-2021 Mark H. P. Lord

#include "StdioLog.h"
#include <stdio.h>

namespace Prime {

void StdioLog::write(Level level, const char* string)
{
    FILE* stream = getUseStdoutForLevel(level) ? stdout : stderr;

    if (stream != stdout) {
        fflush(stdout);
    }

    fputs(string, stream);
}

}
