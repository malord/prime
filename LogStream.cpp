// Copyright 2000-2021 Mark H. P. Lord

#include "LogStream.h"

namespace Prime {

LogStream::LogStream(Log* log, Log::Level level)
    : _log(log)
    , _level(level)
{
}

LogStream::~LogStream()
{
    if (_log) {
        flush(_log);
    }
}

void LogStream::setLog(Log* log, Log::Level level)
{
    _log = log;
    _level = level;
}

ptrdiff_t LogStream::writeSome(const void* memory, size_t maximumBytes, Log* log)
{
    (void)log;

    const char* ptr = (const char*)memory;
    const char* end = ptr + maximumBytes;

    for (;;) {
        const char* begin = ptr;
        while (ptr != end && *ptr != '\r' && *ptr != '\n') {
            ++ptr;
        }

        _line.append(begin, ptr);

        if (ptr == end) {
            break;
        }

        if (*ptr == '\r') {
            ++ptr;
            continue;
        }

        PRIME_DEBUG_ASSERT(*ptr == '\n');
        ++ptr;
        flush(_log);
    }

    return maximumBytes;
}

bool LogStream::close(Log* log)
{
    return flush(log);
}

bool LogStream::flush(Log* log)
{
    if (!_line.empty()) {
        log->log(_level, "%s", _line.c_str());
        _line.clear();
    }

    return true;
}
}
