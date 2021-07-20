// Copyright 2000-2021 Mark H. P. Lord

#include "TextLog.h"
#include "Clocks.h"
#include "DateTime.h"
#include "ScopedPtr.h"
#include "StringUtils.h"

namespace {

using namespace Prime;

#ifdef PRIME_OS_WINDOWS

void StripExecutableExtension(char* path)
{
    char* extension = (char*)StringLastComponent(path, PRIME_PATH_SEPARATORS ".");

    if (extension == path || extension[-1] != '.') {
        return;
    }

    // I could potentially cut any extension in the PATHEXT environment variable. But I don't.
    if (ASCIIEqualIgnoringCase(extension, "exe") || ASCIIEqualIgnoringCase(extension, "com")) {
        extension[-1] = 0;
    }
}

#else

void StripExecutableExtension(char*)
{
}

#endif
}

namespace Prime {

PRIME_DEFINE_UID_CAST(TextLog)

TextLog::TextLog()
{
    _timePrefix = false;

    // The default is LevelMin so that an application will log everything until it has command line parsing.
    _level = LevelMin;

    _runningInDebugger = -1;

    _globalPrefix.resize(0);

    static const char* defaultLevelPrefix[LevelMax - LevelMin + 1] = {
        NULL, // Trace
        NULL, // Verbose
        NULL, // Output
        NULL, // Info
        "NOTE", // Note
        "WARNING",
        "DEVELOPER WARNING",
        "ERROR",
        "RUNTIME ERROR",
        "ERROR" // Fatal Error (don't need to distinguish this from a regular error... the program quits)
    };

    memcpy(_levelPrefix, defaultLevelPrefix, sizeof(_levelPrefix));

    static bool defaultLevelShouldShowAppName[LevelMax - LevelMin + 1] = {
        true, // Trace
        true, // Verbose
        false, // Output
        true, // Info
        true, // Note
        true, // Warning
        true, // DeveloperWarning
        true, // Error
        true, // RuntimeError
        true, // FatalError
    };

    memcpy(_levelUsesGlobalPrefix, defaultLevelShouldShowAppName, sizeof(_levelUsesGlobalPrefix));
}

TextLog::~TextLog()
{
}

void TextLog::increaseVerbosity()
{
    if (getLevel() <= LevelTrace) {
        SetDeveloperMode(true);
    } else if (getLevel() <= LevelVerbose) {
        setLevel(LevelTrace);
    } else {
        setLevel(LevelVerbose);
    }
}

void TextLog::setGlobalPrefix(const char* globalPrefix)
{
    _globalPrefix = globalPrefix;
}

const char* TextLog::getGlobalPrefix() const
{
    return _globalPrefix.c_str();
}

void TextLog::setApplicationName(const char* name)
{
    char applicationName[64];
    StringCopy(applicationName, sizeof(applicationName), StringLastComponent(name, PRIME_PATH_SEPARATORS));
    StripExecutableExtension(applicationName);

    setGlobalPrefix(applicationName);
}

void TextLog::enableGlobalPrefixForAlertLevelsOnly()
{
    for (Level level = LevelMin; level <= LevelMax; level = (Level)(level + 1)) {
        setGlobalPrefixEnabledForLevel(level, level >= LevelNote);
    }
}

void TextLog::setGlobalPrefixEnabledForLevel(Level level, bool prefix)
{
    PRIME_ASSERT(isValidLevel(level));

    _levelUsesGlobalPrefix[level - LevelMin] = prefix;
}

bool TextLog::getGlobalPrefixEnabledForLevel(Level level) const
{
    PRIME_ASSERT(isValidLevel(level));

    return _levelUsesGlobalPrefix[level - LevelMin];
}

void TextLog::setLevelPrefix(Level level, const char* prefix)
{
    PRIME_ASSERT(isValidLevel(level));

    _levelPrefix[level - LevelMin] = prefix;
}

const char* TextLog::getLevelPrefix(Level level)
{
    PRIME_ASSERT(isValidLevel(level));

    return _levelPrefix[level - LevelMin];
}

bool TextLog::logVA(Level level, const char* format, va_list argptr)
{
    PRIME_ASSERT(isValidLevel(level));

    if (!isLevelEnabled(level)) {
        return false;
    }

    char stack[PRIME_MIN(PRIME_BIG_STACK_BUFFER_SIZE, 512)];
    size_t nextBufferSize = sizeof(stack);

    const size_t maxBufferSize = 64 * 1024;

    for (;;) {
        char* buffer;
        size_t bufferSize;
        ScopedArrayPtr<char> alloced;

        if (nextBufferSize == sizeof(stack)) {
            buffer = stack;
        } else {
            alloced.reset(new char[nextBufferSize]);
            buffer = alloced.get();
            if (!alloced.get()) {
                break;
            }
        }

        bufferSize = nextBufferSize - 1; // Reserve room for a newline
        nextBufferSize *= 2;
        buffer[0] = 0;

        va_list argptr2;
        PRIME_VA_COPY(argptr2, argptr);

        bool fit = true;

        if (level >= LevelFatalError) {
            fit = fit && StringAppend(buffer, bufferSize, "\n");
        }

        if (_timePrefix) {
            UnixTime now = Clock::getCurrentTime();
            fit = fit && Clock::unixTimeToLocalDateTime(now).toISO8601(buffer, bufferSize, " ", "");
            fit = fit && StringAppend(buffer, bufferSize, " ");
        }

        if (!_globalPrefix.empty() && getGlobalPrefixEnabledForLevel(level) && shouldLevelHaveGlobalPrefix(level)) {
            fit = fit && StringAppend(buffer, bufferSize, _globalPrefix.c_str());
            fit = fit && StringAppend(buffer, bufferSize, ": ");
        }

        fit = fit && appendLevelPrefix(buffer, bufferSize, level);

        char* newlinesStart = buffer + strlen(buffer);

        fit = fit && appendLog(buffer, bufferSize, level, format, argptr2);

        char* newlinesEnd = newlinesStart;
        while (*newlinesEnd == '\n') {
            ++newlinesEnd;
        }

        if (newlinesEnd != newlinesStart) {
            memmove(buffer + (newlinesEnd - newlinesStart), buffer, newlinesStart - buffer);
            for (ptrdiff_t i = 0; i != newlinesEnd - newlinesStart; ++i) {
                buffer[i] = '\n';
            }
        }

        if (!StringEndsWith(buffer, '\n')) {
            fit = fit && StringAppend(buffer, bufferSize + 1, "\n");
        }

        va_end(argptr2);

        if (fit || nextBufferSize >= maxBufferSize) {
            write(level, buffer);
            break;
        }
    }

    return false;
}

bool TextLog::appendLevelPrefix(char* buffer, size_t bufferSize, Level level)
{
    const char* levelPrefix = getLevelPrefix(level);
    if (!levelPrefix || !*levelPrefix) {
        return true;
    }

    if (level >= LevelWarning) {
        bool fit = StringAppend(buffer, bufferSize, "*** ");

        char* end = buffer + strlen(buffer);

        fit = fit && StringAppend(buffer, bufferSize, levelPrefix);

        ASCIIToUpperInPlace(end);

        fit = fit && StringAppend(buffer, bufferSize, " ***: ");

        return fit;

    } else {
        bool fit = StringAppend(buffer, bufferSize, levelPrefix);

        fit = fit && StringAppend(buffer, bufferSize, ": ");

        return fit;
    }
}

bool TextLog::appendLog(char* buffer, size_t bufferSize, Level level, const char* format, va_list argptr)
{
    (void)level;
    return StringAppendFormatVA(buffer, bufferSize, format, argptr);
}

bool TextLog::isOutputATTYForLevel(Level) const
{
    return false;
}

bool TextLog::shouldLevelHaveGlobalPrefix(Level level) const
{
#ifndef PRIME_FINAL
    if (_runningInDebugger < 0) {
        _runningInDebugger = IsDebuggerAttached() ? 1 : 0;
    }

    if (_runningInDebugger) {
        return false;
    }
#endif

    return !isOutputATTYForLevel(level);
}
}
