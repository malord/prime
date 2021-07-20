// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STREAMLOG_H
#define PRIME_STREAMLOG_H

#include "Mutex.h"
#include "Stream.h"
#include "TextLog.h"

namespace Prime {

/// A Log which writes to a file. For maximum performance, use this via a LogThreader. Writes to the Stream are
/// serialised by an internal mutex, but if you need to write to the Stream independently of a StreamLog, wrap
/// the Stream in a ThreadSafeStream. There's no StringLog any more - use a StreamLog with a StringStream.
/// For logging to a file re-used across sessions, FileLog provides more features.
class PRIME_PUBLIC StreamLog : public TextLog {
public:
    StreamLog();

    explicit StreamLog(Stream* stream, Log* errorLog = Log::getNullLog(), bool threadSafe = true);

    ~StreamLog();

    bool init(Stream* stream, Log* errorLog = Log::getNullLog(), bool threadSafe = true);

protected:
    virtual void write(Level level, const char* string) PRIME_OVERRIDE;

private:
    RecursiveMutex _mutex;

    RefPtr<Stream> _stream;
    RefPtr<Log> _errorLog;
};
}

#endif
