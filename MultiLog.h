// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MULTILOG_H
#define PRIME_MULTILOG_H

#include "Log.h"

namespace Prime {

/// Route log messages to multiple Logs.
class PRIME_PUBLIC MultiLog : public Log {
public:
    MultiLog()
        : _logCount(0)
    {
    }

    /// Clear the list of Logs.
    void reset();

    void addLog(Log* log);

    bool replace(Log* log, Log* with);

    virtual bool logVA(Level level, const char* format, va_list argptr) PRIME_OVERRIDE;

    operator Log* () { return this; }

private:
    enum { size = 6 };

    // If you add more than size Logs, then the size+1'th will be another MultiLog thereby creating
    // a linked list of MultiLogs, allowing as many Logs as are needed.
    RefPtr<Log> _log[size + 1];
    size_t _logCount;

    PRIME_UNCOPYABLE(MultiLog);
};
}

#endif
