// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STDIOLOG_H
#define PRIME_STDIOLOG_H

#include "ConsoleLog.h"

namespace Prime {

/// A Log implementation that writes to stdout and stderr.
class PRIME_PUBLIC StdioLog : public ConsoleLog {
protected:
    virtual void write(Level level, const char* string) PRIME_OVERRIDE;
};
}

#endif
