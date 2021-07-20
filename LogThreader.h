// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_LOGTHREADER_H
#define PRIME_LOGTHREADER_H

#include "Condition.h"
#include "Log.h"
#include "LogRecorder.h"
#include "Mutex.h"
#include "Thread.h"

namespace Prime {

/// Queues log messages and passes them to an underlying Log on a thread, returning to the caller as quickly as
/// possible. Wrap slow Log implementations (e.g., FileLog, or a theoretical RemoteLog) in this to alleviate
/// performance bottlenecks on user interface threads.
class PRIME_PUBLIC LogThreader : public Log {
public:
    LogThreader();

    ~LogThreader();

    bool init(Log* underlyingLog, Log* errorLog = NULL);

    void close();

    /// Log implementation.
    virtual bool logVA(Level level, const char* format, va_list argptr) PRIME_OVERRIDE;

private:
    void thread();

    void flush(LogRecorder* recorder);

    enum { stackSize = 64 * 1024 };

    RefPtr<Log> _log;
    Thread _thread;

    RecursiveMutex _recorderMutex;
    typedef RecursiveMutex::ScopedLock RecorderMutexLock;

    RecursiveMutex _logMutex;
    typedef RecursiveMutex::ScopedLock LogMutexLock;

    LogRecorder _recorder;
    volatile bool _quit;

    Condition _itemAdded;

    bool _initialised;
};
}

#endif
