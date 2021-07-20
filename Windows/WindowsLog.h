// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSLOG_H
#define PRIME_WINDOWS_WINDOWSLOG_H

#include "../ANSILog.h"
#include <stdio.h>

namespace Prime {

/// A Log implementation that writes to stdout/stderr if available, otherwise via OutputDebugString. If
/// stdout/stderr are attached to the console, writes directly to the console and colourises the output.
class PRIME_PUBLIC WindowsLog : public ANSILog {
public:
    WindowsLog();

    /// Call this if you allocate a console or redirect stdout/stderr.
    void consoleChanged();

    /// ANSILog implementation.
    virtual bool isColourSupportedForLevel(Level level) const PRIME_OVERRIDE;
    virtual bool doesTerminalHaveDarkBackground() const PRIME_OVERRIDE;

protected:
    virtual void write(Level level, const char* string) PRIME_OVERRIDE;
    virtual bool isOutputATTYForLevel(Level level) const PRIME_OVERRIDE;

private:
    bool _consoleAttached;

    FILE* getStreamForLevel(Level level) const;
};
}

#endif
