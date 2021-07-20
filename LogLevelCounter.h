// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_LogLevelCounter_H
#define PRIME_LogLevelCounter_H

#include "Log.h"

namespace Prime {

/// Counts how many of each kind of log level occur. Does not log anything, so you have to use this class alongside
/// MultiLog.
class PRIME_PUBLIC LogLevelCounter : public Log {
public:
    LogLevelCounter()
    {
        std::fill(_counts, _counts + PRIME_COUNTOF(_counts), 0);
    }

    /// Clear the counts.
    void reset();

    virtual bool logVA(Level level, const char* format, va_list argptr) PRIME_OVERRIDE;

    int getCount(Log::Level level) const
    {
        if (level < Log::LevelNone || level > Log::LevelFatalError) {
            return 0;
        }

        return _counts[level - Log::LevelNone];
    }

    int getErrorCount() const
    {
        return getCount(Log::LevelError) + getCount(Log::LevelFatalError) + getCount(Log::LevelRuntimeError);
    }

private:
    int _counts[Log::LevelFatalError - Log::LevelNone + 1];

    PRIME_UNCOPYABLE(LogLevelCounter);
};
}

#endif
