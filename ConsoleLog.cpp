// Copyright 2000-2021 Mark H. P. Lord

#include "ConsoleLog.h"
#include <string.h>

namespace Prime {

PRIME_DEFINE_UID_CAST(ConsoleLog)

ConsoleLog::ConsoleLog()
{
    static bool defaultLevelShouldUseStdout[LevelMax - LevelMin + 1] = {
        false, // Trace
        false, // Verbose
        true, // Output
        false, // Information
        false, // Note
        false, // Warning
        false, // DeveloperWarning
        false, // Error
        false, // RuntimeError
        false, // FatalError
    };

    memcpy(_levelUsesStdout, defaultLevelShouldUseStdout, sizeof(_levelUsesStdout));

    _useOutputDebugString = false;
    _coloursEnabled = -1; // Autodetect
}

void ConsoleLog::setUseStdoutForLevel(Level level, bool useStdout)
{
    PRIME_ASSERT(isValidLevel(level));

    _levelUsesStdout[level - LevelMin] = useStdout;
}

bool ConsoleLog::getUseStdoutForLevel(Level level) const
{
    PRIME_ASSERT(isValidLevel(level));

    return _levelUsesStdout[level - LevelMin];
}

void ConsoleLog::setUseStdoutForAllLevels()
{
    for (Level level = LevelMin; level <= LevelMax; level = (Level)(level + 1)) {
        setUseStdoutForLevel(level, true);
    }
}

void ConsoleLog::setColourEnabled(bool useColours)
{
    _coloursEnabled = useColours ? 1 : 0;
}

bool ConsoleLog::shouldLevelHaveGlobalPrefix(Level level) const
{
    if (_coloursEnabled > 0) {
        return false;
    }

    return TextLog::shouldLevelHaveGlobalPrefix(level);
}
}
