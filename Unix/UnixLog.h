// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXLOG_H
#define PRIME_UNIX_UNIXLOG_H

#include "../ANSILog.h"
#include <stdio.h>

namespace Prime {

/// A Log implementation that writes to stdout or stderr. Supports ANSI terminal colours when writing to a TTY.
class PRIME_PUBLIC UnixLog : public ANSILog {
public:
    UnixLog();

    /// ANSILog implementation.
    virtual bool isColourSupportedForLevel(Level level) const PRIME_OVERRIDE;
    virtual bool doesTerminalHaveDarkBackground() const PRIME_OVERRIDE;

protected:
    virtual void write(Level level, const char* string) PRIME_OVERRIDE;
    virtual bool isOutputATTYForLevel(Level level) const PRIME_OVERRIDE;

private:
    FILE* getStreamForLevel(Level level) const;
};
}

#endif
