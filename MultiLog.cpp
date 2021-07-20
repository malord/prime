// Copyright 2000-2021 Mark H. P. Lord

#include "MultiLog.h"
#include <algorithm>

namespace Prime {

void MultiLog::reset()
{
    if (_logCount) {
        std::fill(_log, _log + _logCount, RefPtr<Log>());
        _logCount = 0;
    }
}

void MultiLog::addLog(Log* log)
{
    if (_logCount < PRIME_COUNTOF(_log) - 1) {
        _log[_logCount++] = log;
        return;
    }

    RefPtr<Log>& lastLog = _log[PRIME_COUNTOF(_log) - 1];
    if (!lastLog) {
        lastLog = PassRef(new MultiLog);
        _logCount++;
    }

    PRIME_DEBUG_ASSERT(_logCount == PRIME_COUNTOF(_log));

    static_cast<MultiLog*>(lastLog.get())->addLog(log);
}

bool MultiLog::logVA(Level level, const char* format, va_list argptr)
{
    if (!_logCount) {
        return false;
    }

    bool handled = false;

    for (size_t i = 0; i != _logCount; ++i) {
        va_list argptr2;
        PRIME_VA_COPY(argptr2, argptr);

        if (_log[i]->logVA(level, format, argptr2)) {
            handled = true;
        }

        va_end(argptr2);
    }

    return handled;
}

bool MultiLog::replace(Log* log, Log* with)
{
    for (size_t i = 0; i != _logCount; ++i) {
        if (i != PRIME_COUNTOF(_log) - 1) {
            if (_log[i] == log) {
                _log[i] = with;
                return true;
            }
        } else {
            return (static_cast<MultiLog*>(_log[i].get()))->replace(log, with);
        }
    }

    return false;
}
}
