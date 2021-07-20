// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DOWNGRADELOG_H
#define PRIME_DOWNGRADELOG_H

#include "Log.h"

namespace Prime {

//
// DowngradeLog
//

/// A Log implementation that remaps LogLevels written to an underlying Log. This can be used to convert errors
/// in to warnings or debug output (i.e., to "downgrade" a log message) or to restrict logs to a minimum level.
class PRIME_PUBLIC DowngradeLog : public Log {
public:
    DowngradeLog();

    explicit DowngradeLog(Log* underlyingLog);

    DowngradeLog(Log* underlyingLog, Level maxLevel);

    void setLog(Log* underlyingLog) { _underlyingLog = underlyingLog; }

    Log* getLog() const { return _underlyingLog; }

    /// Remaps LogLevels > maxLevel to maxLevel and maps the remaining levels to themselves.
    DowngradeLog& setMaxLevel(Level maxLevel);

    /// Remaps LogLevels < maxLevel to LevelNone.
    DowngradeLog& setMinLevel(Level minLevel);

    void resetMappings();

    /// Allows you to use a DowngradeLog directly in an expression, e.g.,
    /// `stream.open(path, Stream::openRead, DowngradeLog(log, Log::LevelWarning))`, where if the open fails, a
    /// warning is produced rather than an error.
    operator Log*() { return this; }

    Log* get() { return this; }

    DowngradeLog* operator->() { return this; }

    // Log implementation.
    virtual bool logVA(Level level, const char* format, va_list argptr) PRIME_OVERRIDE;

private:
    operator void*() { return NULL; }

    RefPtr<Log> _underlyingLog;
    Level _map[LevelMax - LevelMin + 1];

    PRIME_UNCOPYABLE(DowngradeLog);
};

//
// TraceLog
//

/// Utility wrapper around DowngradeLog that downgrades to LevelTrace.
class TraceLog : public DowngradeLog {
public:
    explicit TraceLog(Log* underlyingLog)
        : DowngradeLog(underlyingLog, Log::LevelTrace) {};
};

//
// VerboseLog
//

/// Utility wrapper around DowngradeLog that downgrades to LevelVerbose.
class VerboseLog : public DowngradeLog {
public:
    explicit VerboseLog(Log* underlyingLog)
        : DowngradeLog(underlyingLog, Log::LevelVerbose) {};
};

//
// WarningLog
//

/// Utility wrapper around DowngradeLog that downgrades to LevelWarning.
class WarningLog : public DowngradeLog {
public:
    explicit WarningLog(Log* underlyingLog)
        : DowngradeLog(underlyingLog, Log::LevelWarning) {};
};
}

#endif
