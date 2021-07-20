// Copyright 2000-2021 Mark H. P. Lord

#include "StreamLog.h"
#include <string.h>

namespace Prime {

StreamLog::StreamLog()
{
}

StreamLog::StreamLog(Stream* stream, Log* errorLog, bool threadSafe)
{
    init(stream, errorLog, threadSafe);
}

StreamLog::~StreamLog()
{
}

bool StreamLog::init(Stream* stream, Log* errorLog, bool threadSafe)
{
    PRIME_ASSERT(errorLog != NULL);

    if (threadSafe && !_mutex.init(errorLog, "StreamLog mutex")) {
        return false;
    }

    _stream = stream;
    _errorLog = errorLog;

    return true;
}

void StreamLog::write(Level level, const char* string)
{
    (void)level;

    RecursiveMutex::ScopedLock lock;
    if (_mutex.isInitialised()) {
        lock.lock(&_mutex);
    }

    _stream->writeExact(string, strlen(string), _errorLog.get());
}
}
