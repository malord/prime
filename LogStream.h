// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_LOGSTREAM_H
#define PRIME_LOGSTREAM_H

#include "Log.h"
#include "Stream.h"

namespace Prime {

/// A Stream which writes lines to a Log
class PRIME_PUBLIC LogStream : public Stream {
public:
    explicit LogStream(Log* log = NULL, Log::Level level = Log::LevelTrace);

    ~LogStream();

    void setLog(Log* log, Log::Level level);

    virtual ptrdiff_t writeSome(const void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;

private:
    RefPtr<Log> _log;
    Log::Level _level;

    std::string _line;
};
}

#endif
