// Copyright 2000-2021 Mark H. P. Lord

#include "UnixLog.h"
#include "../ScopedPtr.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace Prime {

UnixLog::UnixLog()
{
#ifdef PRIME_OS_IOS
    setColourEnabled(false);
#endif

#if defined(PRIME_OS_MAC) && !defined(PRIME_FINAL)
    if (IsDebuggerAttached()) {
        setColourEnabled(false);
    }
#endif
}

FILE* UnixLog::getStreamForLevel(Level level) const
{
    return getUseStdoutForLevel(level) ? stdout : stderr;
}

void UnixLog::write(Level level, const char* string)
{
    FILE* stream = getStreamForLevel(level);

    if (stream != stdout) {
        fflush(stdout);
    }

    fputs(string, stream);
}

bool UnixLog::isColourSupportedForLevel(Level level) const
{
    return isOutputATTYForLevel(level);
}

bool UnixLog::isOutputATTYForLevel(Level level) const
{
    bool value;
    if (!getCachedIsATTYForLevel(level, value)) {
        value = isatty(fileno(getStreamForLevel(level))) ? true : false;
        setCachedIsATTYForLevel(level, value);
    }

    return value;
}

bool UnixLog::doesTerminalHaveDarkBackground() const
{
    // Unknown.
    return false;
}
}
