// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PREFIXLOG_H
#define PRIME_PREFIXLOG_H

#include "Log.h"
#include "StringView.h"

namespace Prime {

/// A Log implementation that prepends a prefix to each log message before forwarding them on to another Log.
/// For example: `stream.openForRead(path, PrefixLog(log, path))` will prefix the path to any errors.
class PRIME_PUBLIC PrefixLog : public Log {
public:
    PrefixLog();

    /// PrefixLog(log, "ABC") will result in a prefix of "ABC: " if addSeparator is true.
    explicit PrefixLog(Log* underlyingLog, StringView prefix, bool addSeparator = true);

    ~PrefixLog();

    void init(Log* underlyingLog, StringView prefix, bool addSeparator = true);

    void setLog(Log* underlyingLog) { _underlyingLog = underlyingLog; }

    Log* getLog() const { return _underlyingLog; }

    /// setPrefix("ABC") will result in a prefix of "ABC: " if addSeparator is true.
    void setPrefix(StringView newPrefix, bool addSeparator = true);

    void clearPrefix();

    /// Allows you to do, e.g., `stream.open(path, Stream::openRead, PrefixLog(log, path))`.
    operator Log*() { return this; }

    Log* get() { return this; }

    PrefixLog* operator->() { return this; }

    // Log implementation.
    virtual bool logVA(Level level, const char* format, va_list argptr) PRIME_OVERRIDE;

private:
    void fixPrefix(bool addSeparator);

    operator void*() { return NULL; }

    std::string _prefix;
    RefPtr<Log> _underlyingLog;

    PRIME_UNCOPYABLE(PrefixLog);
};
}

#endif
