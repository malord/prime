// Copyright 2000-2021 Mark H. P. Lord

#include "DowngradeLog.h"
#include "ScopedPtr.h"

namespace Prime {

DowngradeLog::DowngradeLog()
{
    resetMappings();
}

DowngradeLog::DowngradeLog(Log* underlyingLog, Level level)
{
    setLog(underlyingLog);
    setMaxLevel(level);
}

DowngradeLog::DowngradeLog(Log* underlyingLog)
{
    resetMappings();
    setLog(underlyingLog);
}

DowngradeLog& DowngradeLog::setMaxLevel(Level maxLevel)
{
    for (int level = (int)LevelMin; level <= (int)LevelMax; ++level) {
        _map[level - LevelMin] = (Level)PRIME_MIN((int)maxLevel, level);
    }
    return *this;
}

DowngradeLog& DowngradeLog::setMinLevel(Level minLevel)
{
    for (int level = (int)LevelMin; level <= (int)LevelMax; ++level) {
        _map[level - LevelMin] = level < minLevel ? LevelNone : (Level)level;
    }
    return *this;
}

void DowngradeLog::resetMappings()
{
    for (int level = (int)LevelMin; level <= (int)LevelMax; ++level) {
        _map[level - LevelMin] = (Level)level;
    }
}

bool DowngradeLog::logVA(Level level, const char* format, va_list argptr)
{
    if (!_underlyingLog) {
        return false;
    }

    Level newLevel = _map[PRIME_CLAMP(level, LevelMin, LevelMax) - LevelMin];

    if (newLevel == LevelNone) {
        return level == LevelRuntimeError;
    }

    return _underlyingLog->logVA(newLevel, format, argptr) || (level == LevelRuntimeError && newLevel != level);
}
}
