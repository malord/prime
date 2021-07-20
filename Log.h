// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_LOG_H
#define PRIME_LOG_H

#include "RefCounting.h"
#include "UIDCast.h"
#include <string>

namespace Prime {

//
// Log
//

/// Provides an interface through which log messages can be written. By accepting a pointer to a Log instance,
/// libraries can report errors, warnings, notes, debug output, progress output in a way that the application
/// can filter and re-route.
class PRIME_PUBLIC Log : public RefCounted {
    PRIME_DECLARE_UID_CAST_BASE(0x3aea75ee, 0x88fc4013, 0x9f7b017e, 0x7826704a)

public:
    enum Level {
        /// Do not log.
        LevelNone = -3,

        /// Debug output (where you might otherwise use printf() or OutputDebugString()).
        LevelTrace = -2,

        /// Verbose logging can be enabled by a regular users and should report what the application is doing.
        LevelVerbose = -1,

        /// The application's actual output.
        LevelOutput = 0,

        /// Additional information the user should see. Less important than a note.
        LevelInfo = 1,

        /// Something the user should be informed of, but which shouldn't pause the application to alert them.
        LevelNote = 2,

        /// Alert the user, but processing will continue.
        LevelWarning = 3,

        /// For reporting anything a developer should see but a user should not. Would typically go to syslog.
        LevelDeveloperWarning = 4,

        /// Alert the user and notify them that processing will end (but the application will continue).
        LevelError = 5,

        /// Alert the user of an internal error that has probably corrupted the application. The application may
        /// immediately terminate.
        LevelRuntimeError = 6,

        /// Alert the user that the application must exit due to an error. A fatal error differs from an error in
        /// that a graphical application would probably display an alert box for a fatal error.
        LevelFatalError = 7,

        LevelMin = LevelTrace,
        LevelMax = LevelFatalError
    };

    inline static bool isValidLevel(int level)
    {
        return level >= LevelMin && level <= LevelMax;
    }

    /// The global Log is for application-global messages. Rather than use the global Log directly, functions
    /// which need to write log messages should take a pointer to a Log as a parameter and write through that
    /// (allowing logging to be overridden as required). The application is responsible for setting the global
    /// Log, otherwise the result of writing to the Log returned by this function is undefined (but it won't
    /// crash).
    static Log* getGlobal() { return _global ? _global : getNullLog(); }

    static void setGlobal(Log* log) { _global = log; }

    /// Returns a do-nothing implementation of Log that can be used for discarding messages.
    static Log* getNullLog();

    virtual ~Log();

    /// Returns true if the application handled a runtime error or fatal error, e.g., if the user was presented
    /// with a dialogue box and they chose to continue. If false is returned, handling of the error goes up the
    /// caller chain.
    virtual bool logVA(Level level, const char* format, va_list argptr) = 0;

    /// Wrapper around logVA.
    bool log(Level level, const char* format, ...);

    /// Wrapper around logVA.
    bool log(Level level, const std::string& string);

    void trace(const char* format, ...);
    void developerWarning(const char* format, ...);
    void verbose(const char* format, ...);
    void output(const char* format, ...);
    void info(const char* format, ...);
    void note(const char* format, ...);
    void warning(const char* format, ...);
    void error(const char* format, ...);
    void runtimeError(const char* format, ...);
    void fatalError(const char* format, ...);
    void PRIME_NORETURN exitError(const char* format, ...);

    void trace(const std::string& string);
    void developerWarning(const std::string& string);
    void verbose(const std::string& string);
    void output(const std::string& string);
    void info(const std::string& string);
    void note(const std::string& string);
    void warning(const std::string& string);
    void error(const std::string& string);
    void runtimeError(const std::string& string);
    void fatalError(const std::string& string);
    void PRIME_NORETURN exitError(const std::string& string);

    void logErrno(int errorNumber, const char* cause = NULL, Level level = LevelError);

#ifdef PRIME_OS_WINDOWS
    void logWindowsError(unsigned long errorNumber, const char* cause = NULL, Level level = LevelError);
#endif

    void PRIME_NORETURN exitError();

private:
    static Log* _global;
};

//
// Extend the global debug logging functions with support for std::string.
//

PRIME_PUBLIC void Trace(const std::string& string);
PRIME_PUBLIC void DeveloperWarning(const std::string& string);
PRIME_PUBLIC void RuntimeError(const std::string& string);

//
// Other functions
//

PRIME_PUBLIC void PRIME_NORETURN ExitWithErrorCode();

}

#endif
