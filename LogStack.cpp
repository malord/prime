// Copyright 2000-2021 Mark H. P. Lord

#include "LogStack.h"
#include <algorithm>

namespace Prime {

void LogStack::clear()
{
    _logs.clear();
}

void LogStack::push(Log* log)
{
    _logs.push_back(log);
}

RefPtr<Log> LogStack::pop()
{
    if (!PRIME_GUARD(!_logs.empty())) {
        return NULL;
    }

    RefPtr<Log> outgoing = _logs.back();
    _logs.pop_back();
    return outgoing;
}

bool LogStack::logVA(Level level, const char* format, va_list argptr)
{
    if (_logs.empty()) {
        return false;
    }

    return _logs.back()->logVA(level, format, argptr);
}
}
