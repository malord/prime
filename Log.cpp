// Copyright 2000-2021 Mark H. P. Lord

#include "Log.h"
#ifdef PRIME_OS_WINDOWS
#include "Windows/WindowsConfig.h"
#endif
#include "ScopedPtr.h"
#include "StringUtils.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

namespace Prime {

//
// Log
//

PRIME_DEFINE_UID_CAST_BASE(Log)

Log* Log::_global = NULL;

namespace {

    class NullLog : public Log {
    public:
        virtual bool logVA(Level level, const char* format, va_list argptr) PRIME_OVERRIDE
        {
            (void)level;
            (void)format;
            (void)argptr;

            // This means that runtime errors etc are not logged. But ASSERT etc. are global, so as long as
            // the global log is not the null log, this shouldn't be a problem...
            return true;
        }
    };

}

Log* Log::getNullLog()
{
    static NullLog log;
    return &log;
}

Log::~Log()
{
    if (_global == this) {
        _global = NULL;
    }
}

bool Log::log(Level level, const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    bool handled = logVA(level, format, argptr);
    va_end(argptr);
    return handled;
}

bool Log::log(Level level, const std::string& string)
{
    return log(level, "%s", string.c_str());
}

void Log::trace(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    logVA(LevelTrace, format, argptr);
    va_end(argptr);
}

void Log::developerWarning(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    logVA(LevelDeveloperWarning, format, argptr);
    va_end(argptr);
}

void Log::verbose(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    logVA(LevelVerbose, format, argptr);
    va_end(argptr);
}

void Log::output(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    logVA(LevelOutput, format, argptr);
    va_end(argptr);
}

void Log::info(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    logVA(LevelInfo, format, argptr);
    va_end(argptr);
}

void Log::note(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    logVA(LevelNote, format, argptr);
    va_end(argptr);
}

void Log::warning(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    logVA(LevelWarning, format, argptr);
    va_end(argptr);
}

void Log::error(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    logVA(LevelError, format, argptr);
    va_end(argptr);
}

void Log::runtimeError(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    bool handled = logVA(LevelRuntimeError, format, argptr);
    va_end(argptr);
    if (!handled) {
        PRIME_DEBUGGER();
    }
}

void Log::fatalError(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    logVA(LevelFatalError, format, argptr);
    va_end(argptr);
}

void Log::exitError()
{
    ExitWithErrorCode();
}

void Log::exitError(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    logVA(LevelFatalError, format, argptr);
    va_end(argptr);

    exitError();
}

void Log::trace(const std::string& string)
{
    log(LevelTrace, string);
}

void Log::developerWarning(const std::string& string)
{
    log(LevelDeveloperWarning, string);
}

void Log::verbose(const std::string& string)
{
    log(LevelVerbose, string);
}

void Log::output(const std::string& string)
{
    log(LevelOutput, string);
}

void Log::info(const std::string& string)
{
    log(LevelInfo, string);
}

void Log::note(const std::string& string)
{
    log(LevelNote, string);
}

void Log::warning(const std::string& string)
{
    log(LevelWarning, string);
}

void Log::error(const std::string& string)
{
    log(LevelError, string);
}

void Log::runtimeError(const std::string& string)
{
    log(LevelRuntimeError, string);
}

void Log::fatalError(const std::string& string)
{
    log(LevelFatalError, string);
}

void Log::exitError(const std::string& string)
{
    log(LevelFatalError, string);
    exitError();
}

void Log::logErrno(int errorNumber, const char* cause, Level level)
{
    if (cause && *cause) {
        log(level, "%s: %s", cause, strerror(errorNumber));
    } else {
        log(level, "%s", strerror(errorNumber));
    }
}

#ifdef PRIME_OS_WINDOWS
#ifdef PRIME_OS_XBOX
void Log::logWindowsError(unsigned long errorNumber, const char* cause, Level level)
{
    if (cause && *cause) {
        log(level, "%s: System error 0x%08lx", cause, errorNumber);
    } else {
        log(level, "System error 0x%08lx", errorNumber);
    }
}
#else
void Log::logWindowsError(unsigned long errorNumber, const char* cause, Level level)
{
    // FORMAT_MESSAGE_ALLOCATE_BUFFER is being discouraged by Microsoft.
    for (DWORD size = 128; size <= 65536; size *= 2) {
        SetLastError(0);

        ScopedArrayPtr<TCHAR> message(new TCHAR[size]);
        DWORD result = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            errorNumber,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            message.get(),
            size - 1,
            NULL);

        if (result > 0) {
            if (result >= size - 1) {
                continue;
            }

            std::string charMessage = TCharToChar(message.get());

            if (cause && *cause) {
                log(level, "%s: %s", cause, charMessage.c_str());
            } else {
                log(level, "%s", charMessage.c_str());
            }

            return;
        }

        DWORD error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER) {
            break;
        }
    }

    if (cause && *cause) {
        log(level, "%s: Windows error %lu", cause, (unsigned long int)errorNumber);
    } else {
        log(level, "Windows error %lu", (unsigned long int)errorNumber);
    }
}
#endif
#endif

//
// Developer functions (some of these are declared in Common.h)
//

#ifdef _MSC_VER
#pragma warning(disable : 4645)
#endif

#ifndef PRIME_CUSTOM_ASSERT
void AssertionFailed(const char* file, int line, const char* condition, const char* format, ...) PRIME_NOEXCEPT
{
    const char* filename = StringLastComponent(file, PRIME_PATH_SEPARATORS);

    bool handled;

    if (format && *format) {
        va_list argptr;
        va_start(argptr, format);
        FormatBufferVA<> formatted(format, argptr);
        va_end(argptr);
        handled = Log::getGlobal()->log(Log::LevelRuntimeError, "Assertion (%s) failed (%s, line %d): %s", condition, filename, line, formatted.c_str());
    } else {
        handled = Log::getGlobal()->log(Log::LevelRuntimeError, "Assertion (%s) failed (%s, line %d).", condition, filename, line);
    }

    if (!handled) {
        // To stop this breaking in to the debugger while you're debugging, set the global debuggerEnabled to 0.
        PRIME_DEBUGGER();
    }
}
#endif

void VerifyFailed(const char* file, int line, const char* condition, const char* format, ...) PRIME_NOEXCEPT
{
    const char* filename = StringLastComponent(file, PRIME_PATH_SEPARATORS);

    if (format && *format) {
        va_list argptr;
        va_start(argptr, format);
        FormatBufferVA<> formatted(format, argptr);
        va_end(argptr);
        Log::getGlobal()->log(Log::LevelDeveloperWarning, "Verify (%s) failed (%s, line %d): %s", condition, filename, line, formatted.c_str());
    } else {
        Log::getGlobal()->log(Log::LevelDeveloperWarning, "Verify (%s) failed (%s, line %d).", condition, filename, line);
    }
}

#ifdef _MSC_VER
#pragma warning(default : 4645)
#endif

void Trace(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    Log::getGlobal()->logVA(Log::LevelTrace, format, argptr);
    va_end(argptr);
}

void DeveloperWarning(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    Log::getGlobal()->logVA(Log::LevelDeveloperWarning, format, argptr);
    va_end(argptr);
}

void RuntimeError(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    bool handled = Log::getGlobal()->logVA(Log::LevelRuntimeError, format, argptr);
    va_end(argptr);

    if (!handled) {
        PRIME_DEBUGGER();
    }
}

void Trace(const std::string& string)
{
    Log::getGlobal()->log(Log::LevelTrace, string);
}

void DeveloperWarning(const std::string& string)
{
    Log::getGlobal()->log(Log::LevelDeveloperWarning, string);
}

void RuntimeError(const std::string& string)
{
    Log::getGlobal()->log(Log::LevelRuntimeError, string);
}

void PRIME_NORETURN ExitWithErrorCode()
{
    exit(EXIT_FAILURE);
}
}
