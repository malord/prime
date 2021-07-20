// Copyright 2000-2021 Mark H. P. Lord

#include "PrefixLog.h"
#include "ScopedPtr.h"
#include "StringUtils.h"

namespace Prime {

PrefixLog::PrefixLog()
{
}

PrefixLog::PrefixLog(Log* underlyingLog, StringView prefix, bool addSeparator)
{
    _underlyingLog = underlyingLog;
    StringCopy(_prefix, prefix);
    fixPrefix(addSeparator);
}

PrefixLog::~PrefixLog()
{
}

void PrefixLog::init(Log* underlyingLog, StringView prefix, bool addSeparator)
{
    _underlyingLog = underlyingLog;
    StringCopy(_prefix, prefix);
    fixPrefix(addSeparator);
}

void PrefixLog::setPrefix(StringView newPrefix, bool addSeparator)
{
    StringCopy(_prefix, newPrefix);
    fixPrefix(addSeparator);
}

void PrefixLog::clearPrefix()
{
    _prefix.clear();
}

void PrefixLog::fixPrefix(bool addSeparator)
{
    if (addSeparator && !_prefix.empty()) {
        _prefix += ": ";
    }

    StringReplaceInPlace(_prefix, "%", "%%");
}

bool PrefixLog::logVA(Level level, const char* format, va_list argptr)
{
    if (!_underlyingLog) {
        return false;
    }

    char stack[128];

    if (StringCopy(stack, sizeof(stack), _prefix) && StringAppend(stack, sizeof(stack), format)) {
        return _underlyingLog->logVA(level, stack, argptr);
    }

    return _underlyingLog->logVA(level, (_prefix + format).c_str(), argptr);
}
}
