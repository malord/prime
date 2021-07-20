// Copyright 2000-2021 Mark H. P. Lord

#include "LogThreader.h"

namespace Prime {

LogThreader::LogThreader()
    : _initialised(false)
    , _quit(false)
{
}

LogThreader::~LogThreader()
{
    close();
}

bool LogThreader::init(Log* log, Log* errorLog)
{
    if (_initialised) {
        return true;
    }

    _quit = false;

    if (!errorLog) {
        errorLog = log;
    }

    if (!_recorderMutex.isInitialised() && !_recorderMutex.init(errorLog)) {
        return false;
    }

    if (!_itemAdded.init(&_recorderMutex, errorLog)) {
        return false;
    }

    if (!_logMutex.isInitialised() && !_logMutex.init(errorLog)) {
        return false;
    }

#ifdef PRIME_CXX11_STL
    if (!_thread.create([this] { this->thread(); }, stackSize, errorLog, "LogThreader")) {
        return false;
    }
#else
    if (!_thread.create(MethodCallback(this, &LogThreader::thread), stackSize, errorLog, "LogThreader")) {
        return false;
    }
#endif

    _log = log;
    _initialised = true;

    return true;
}

void LogThreader::close()
{
    if (_initialised) {
        {
            RecorderMutexLock lockRecorder(&_recorderMutex);

            _quit = true;

            _itemAdded.wakeOne();
        }

        _thread.join();

        _itemAdded.close();
        _initialised = false;
    }

    flush(&_recorder);
}

void LogThreader::flush(LogRecorder* recorder)
{
    if (_log) {
        recorder->replay(_log);
    }

    recorder->clear();
}

bool LogThreader::logVA(Level level, const char* format, va_list argptr)
{
    if (!_initialised) {
        return false;
    }

    RecorderMutexLock lockRecorder(&_recorderMutex);

    // Check again for threads that are still logging after we've been shut down.
    if (!_initialised) {
        if (_log) {
            return _log->logVA(level, format, argptr);
        }
        return false;
    }

    _recorder.logVA(level, format, argptr);

    // Runtime and Fatal errors may result in the application terminating, so make sure we've sent all the log
    // items to the underlying log.
    if (level == LevelRuntimeError || level == LevelFatalError || level == LevelDeveloperWarning) {
        LogMutexLock lockLogMutex(&_logMutex);
        flush(&_recorder);
    } else {
        _itemAdded.wakeOne();
    }

    return false;
}

void LogThreader::thread()
{
    bool quit = false;
    do {
        LogMutexLock lockLogMutex;

        LogRecorder taken;
        {
            RecorderMutexLock lockRecorder(&_recorderMutex);

            while (_recorder.empty() && !_quit) {
                _itemAdded.wait(lockRecorder);
            }

            quit = _quit;

            // Take the entire LogRecorder content so we can release the mutex as quickly as possible.
            taken.move(_recorder);

            lockLogMutex.lock(&_logMutex);
        }

        flush(&taken);
    } while (!quit);
}
}
