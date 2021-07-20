// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TEXTLOG_H
#define PRIME_TEXTLOG_H

#include "Log.h"
#include <string>

namespace Prime {

/// Extend Log with methods specific to those which output lines of text.
class PRIME_PUBLIC TextLog : public Log {
    PRIME_DECLARE_UID_CAST(Log, 0x8756e229, 0xa0a44a89, 0xb12cd46c, 0x18301f28)
public:
    TextLog();

    ~TextLog();

    //
    // Date/time prefix
    //

    void setTimePrefix(bool enable) { _timePrefix = enable; }

    bool getTimePrefix() const { return _timePrefix; }

    //
    // Global prefix
    //

    /// Sets a prefix to apply to all log messages where getGlobalPrefixEnabledForLevel() returns true.
    void setGlobalPrefix(const char* globalPrefix);

    const char* getGlobalPrefix() const;

    void setGlobalPrefixEnabledForLevel(Level level, bool prefix);

    bool getGlobalPrefixEnabledForLevel(Level level) const;

    /// Enable the global prefix for LevelNote and higher and disable it for everything else.
    void enableGlobalPrefixForAlertLevelsOnly();

    /// Set the global prefix to the last component of a path (with ".exe" stripped on Windows).
    void setApplicationName(const char* name);

    //
    // Level filtering
    //

    /// The default minimum level is LevelMin.
    void setLevel(Level level) { _level = level; }

    /// You should usually call isLevelEnabled() rather than test the result of this function, since it deals
    /// with developer mode.
    Level getLevel() const { return _level; }

    /// Returns true if the default global log would log this log level.
    bool isLevelEnabled(Level level) { return level >= _level; }

    /// Sets the minimum log level to LevelVerbose, or to LevelTrace if it's already at LevelVerbose or lower.
    /// Using this allows a command line tool to switch to trace mode by specifying verbose mode (usually, -v)
    /// twice (e.g., -v -v or -vv). -vvv enables developer mode.
    void increaseVerbosity();

    //
    // Level specific prefixes (e.g., "WARNING:" and "ERROR:")
    //

    /// Set a level specific prefix. These are set to English defaults. The prefix is not copied.
    void setLevelPrefix(Level level, const char* prefix);

    const char* getLevelPrefix(Level level);

    //
    // Log implementation (derived classes must implement write())
    //

    virtual bool logVA(Level level, const char* format, va_list argptr) PRIME_OVERRIDE;

protected:
    /// The string is guaranteed to be terminated with a newline (a single '\n').
    virtual void write(Level level, const char* string) = 0;

    /// Implementations can optionally override this to customise the log level prefix. Return false if the
    /// prefix did not fit in the buffer.
    virtual bool appendLevelPrefix(char* buffer, size_t bufferSize, Level level);

    /// Allow implementations to override the log message. If this method doesn't append a newline, one is
    /// appended automatically.
    virtual bool appendLog(char* buffer, size_t bufferSize, Level level, const char* format, va_list argptr);

    /// Return true if the output is a TTY. Certain changes occur when writing directly to a TTY, e.g., the
    /// global log prefix is omitted.
    virtual bool isOutputATTYForLevel(Level) const;

    /// Takes in to account whether we're running in a debugger.
    virtual bool shouldLevelHaveGlobalPrefix(Level level) const;

private:
    Level _level;

    bool _timePrefix;

    std::string _globalPrefix;

    const char* _levelPrefix[LevelMax - LevelMin + 1];

    bool _levelUsesGlobalPrefix[LevelMax - LevelMin + 1];

    mutable signed char _runningInDebugger;

    PRIME_UNCOPYABLE(TextLog);
};
}

#endif
