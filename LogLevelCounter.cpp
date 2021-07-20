// Copyright 2000-2021 Mark H. P. Lord

#include "LogLevelCounter.h"
#include <algorithm>

namespace Prime {

void LogLevelCounter::reset()
{
    std::fill(_counts, _counts + PRIME_COUNTOF(_counts), 0);
}

bool LogLevelCounter::logVA(Level level, const char*, va_list)
{
    if (level < Log::LevelNone || level > Log::LevelFatalError) {
        return 0;
    }

    _counts[level - Log::LevelNone] += 1;
    return false;
}

}
