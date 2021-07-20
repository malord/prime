// Copyright 2000-2021 Mark H. P. Lord

#include "CallbackLog.h"

namespace Prime {

void CallbackLog::createMutex()
{
    if (!_mutex.isInitialised()) {
        _mutex.init(Log::getGlobal());
    }
}

void CallbackLog::write(Level level, const char* line)
{
    PRIME_ASSERT(isValidLevel(level));

    RecursiveMutex::ScopedLock lock(&_mutex);

    if (_callback) {
        _callback(line);
    }

    if (_levelCallback) {
        _levelCallback(level, line);
    }
}

void CallbackLog::setCallback(const Callback& callback)
{
    createMutex();
    RecursiveMutex::ScopedLock lock(&_mutex);
    _callback = callback;
}

void CallbackLog::setCallback(const LevelCallback& callback)
{
    createMutex();
    RecursiveMutex::ScopedLock lock(&_mutex);
    _levelCallback = callback;
}

void CallbackLog::clearCallback()
{
    RecursiveMutex::ScopedLock lock(&_mutex);
    _callback = Callback();
    _levelCallback = LevelCallback();
}
}
