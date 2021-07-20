// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ANSILOG_H
#define PRIME_ANSILOG_H

#include "ConsoleLog.h"

namespace Prime {

/// Extends ConsoleLog to use ANSI escape sequences to colourise output.
class PRIME_PUBLIC ANSILog : public ConsoleLog {
public:
    ANSILog();

    ~ANSILog();

    virtual bool isColourSupportedForLevel(Level level) const = 0;

    /// Return true if the terminal background colour is dark, false if it's not (or if not known, since the
    /// light-background colours work OK on a dark screen but not the other way around).
    virtual bool doesTerminalHaveDarkBackground() const = 0;

    bool getBrightColoursEnabled() const { return _brightColours; }

    void setBrightColoursEnabled(bool value) { _brightColours = value; }

protected:
    // ConsoleLog overrides.
    virtual bool appendLevelPrefix(char* buffer, size_t bufferSize, Level level) PRIME_OVERRIDE;
    virtual bool appendLog(char* buffer, size_t bufferSize, Level level, const char* format, va_list argptr) PRIME_OVERRIDE;

    // Caching system to remember whether a Level is a TTY or not.
    bool getCachedIsATTYForLevel(Level level, bool& result) const;
    void setCachedIsATTYForLevel(Level level, bool value) const;

private:
    bool _brightColours;

    mutable int _isTTY[LevelMax - LevelMin + 1];
};
}

#endif
