// Copyright 2000-2021 Mark H. P. Lord

//
// Included by Config.h, and therefore included everywhere.
// Provides assertions, developer logging, memory allocation and other prerequisites.
//
// Options:
// #define PRIME_CUSTOM_ASSERT and optionally PRIME_CUSTOM_ASSERTMSG to override the assertions.
//

// Make sure we're included at the correct point in Config.h.
#include "Config.h"

#ifndef PRIME_COMMON_H
#define PRIME_COMMON_H

#ifdef PRIME_COMPILER_RVALUEREF
// Always want std::move, std::forward
#include <utility>
#endif

#include <limits>

namespace Prime {

#ifndef COUNTOF
#define COUNTOF PRIME_COUNTOF
#endif

//
// Build Mode
//
// There are three build modes:
//
// - Debug, where all assertions and logging (even those which affect performance) are enabled. This is the default
//   build configuration if neither NDEBUG or PRIME_FINAL are defined. PRIME_DEBUG is defined if debug is detected.
//
// - Release, where debug assertions (the PRIME_DEBUG_ prefixed assertions) are disabled but all other assertions are
//   enabled. #define NDEBUG to enable a Release build. Desktop applications would be released in this build mode.
//   Any assertions which have a significant affect on performance (e.g., bounds checking every array access) should
//   be debug assertions to ensure that Release builds have almost optimal performance.
//
// - Final, where all assertions and tests are disabled. #define PRIME_FINAL to enable a Final build. Console games
//   would be released using this build mode.
//

#if defined(PRIME_FINAL)

#if defined(PRIME_DEBUG)
#error PRIME_FINAL and PRIME_DEBUG are mutually exclusive.
#endif

#elif defined(NDEBUG)

#if defined(PRIME_DEBUG)
#error NDEBUG and PRIME_DEBUG are mutually exclusive.
#endif

#else

#if !defined(PRIME_DEBUG)
#define PRIME_DEBUG
#endif

#endif

//
// PRIME_DEBUG_ONLY(x)
// Expands to x only in debug builds, e.g., PRIME_DEBUG_ONLY(Trace("This is a full debug build!"));
// Use this to prevent code that affects performance from appearing in Release builds.
//

#if defined(PRIME_DEBUG)
#define PRIME_DEBUG_ONLY(anything) anything
#else
#define PRIME_DEBUG_ONLY(anything)
#endif

//
// PRIME_UNLESS_FINAL(x)
// Expands to x except in a Final build. e.g., PRIME_UNLESS_FINAL(DeveloperWarning("That ain't right."))
//

#if defined(PRIME_FINAL)
#define PRIME_UNLESS_FINAL(anything)
#else
#define PRIME_UNLESS_FINAL(anything) anything
#endif

//
// Assertions
//
// - PRIME_ASSERT is a standard assertion, which is only stripped from Final builds.
//
// - PRIME_DEBUG_ASSERT is a debug assertion, stripped from Final and Release builds. Debug assertions are used in
//   cases where the assertion will affect the application's performance (e.g., bounds checking every array index).
//
// - PRIME_ALWAYS_ASSERT is never stripped, even from final builds.
//
// MSG-suffixed variants take a printf formatted message to display if the assertion fails:
//
//     PRIME_ASSERTMSG(ptr != NULL, "Failed to allocate buffer (needed %" PRIuPTR" bytes)", needBytes);
//
// Guards (PRIME_GUARD, etc) are designed to be used within if statements. For example, in Debug and Release
// builds, this will raise a developer warning if the pointer is null, but in Final builds it will only check the
// expression, allowing for recovery:
//
//     if (! PRIME_GUARD(ptr != NULL)) {
//         // Handle the situation where we didn't have enough memory
//     }
//
// Expects (PRIME_EXPECT, etc) will report a DeveloperWarning if an expression is false but will not interrupt
// the application. In final builds, the expression is evaluated but the result is not checked. For example:
//
//     // If this fails then carry on, but print a developer warning in Debug and Release build modes.
//     PRIME_EXPECT(dup2(stdoutHandle, stderrHandle) >= 0);
//
// You can disable the Prime assertions and use assert() or some other assertion by defining PRIME_CUSTOM_ASSERT,
// e.g., #define PRIME_CUSTOM_ASSERT assert. Note that the custom assert must act like a function, i.e.,
// (PRIME_CUSTOM_ASSERT(p != NULL), true) must be a valid expression (the C/C++ standards require this of assert()).
// You can also alternatively define PRIME_CUSTOM_ASSERTMSG, which should take a (possibly NULL) string (or variable
// arguments on compilers which support them).
//

#ifndef PRIME_CUSTOM_ASSERT
PRIME_PUBLIC PRIME_ANALYZER_NORETURN void AssertionFailed(const char* file, int line, const char* condition, const char* format, ...) PRIME_NOEXCEPT;
#endif

PRIME_PUBLIC PRIME_ANALYZER_NORETURN void VerifyFailed(const char* file, int line, const char* condition, const char* format, ...) PRIME_NOEXCEPT;

#ifdef PRIME_NO_VA_ARGS
#if defined(PRIME_CUSTOM_ASSERTMSG)
#define PRIME_ALWAYS_ASSERTMSG(cond, msg) PRIME_CUSTOM_ASSERTMSG(cond, msg)
#define PRIME_ALWAYS_GUARDMSG(cond, msg) (PRIME_CUSTOM_ASSERTMSG(cond, msg), (cond))
#elif defined(PRIME_CUSTOM_ASSERT)
#define PRIME_ALWAYS_ASSERTMSG(cond, msg) PRIME_CUSTOM_ASSERT(cond)
#define PRIME_ALWAYS_GUARDMSG(cond, msg) (PRIME_CUSTOM_ASSERT(cond), (cond))
#else
#define PRIME_ALWAYS_ASSERTMSG(cond, msg) ((cond) ? ((void)0) : Prime::AssertionFailed(__FILE__, __LINE__, #cond, msg))
#define PRIME_ALWAYS_GUARDMSG(cond, msg) ((cond) ? 1 : (Prime::AssertionFailed(__FILE__, __LINE__, #cond, msg), 0))
#endif

#define PRIME_NEVER_ASSERTMSG(cond, msg) ((void)0)
#define PRIME_NEVER_GUARDMSG(cond, msg) (cond)

#define PRIME_ALWAYS_EXPECTMSG(cond, msg) ((cond) ? ((void)0) : Prime::VerifyFailed(__FILE__, __LINE__, #cond, msg))
#define PRIME_NEVER_EXPECTMSG(cond, msg) ((void)(cond))
#else
#if defined(PRIME_CUSTOM_ASSERTMSG)
#define PRIME_ALWAYS_ASSERTMSG(cond, ...) PRIME_CUSTOM_ASSERTMSG(cond, __VA_ARGS__)
#define PRIME_ALWAYS_GUARDMSG(cond, ...) (PRIME_CUSTOM_ASSERTMSG(cond, __VA_ARGS__), (cond))
#elif defined(PRIME_CUSTOM_ASSERT)
#define PRIME_ALWAYS_ASSERTMSG(cond, ...) PRIME_CUSTOM_ASSERT(cond)
#define PRIME_ALWAYS_GUARDMSG(cond, ...) (PRIME_CUSTOM_ASSERT(cond), (cond))
#else
#define PRIME_ALWAYS_ASSERTMSG(cond, ...) ((cond) ? ((void)0) : Prime::AssertionFailed(__FILE__, __LINE__, #cond, __VA_ARGS__))
#define PRIME_ALWAYS_GUARDMSG(cond, ...) ((cond) ? 1 : (Prime::AssertionFailed(__FILE__, __LINE__, #cond, __VA_ARGS__), 0))
#endif

#define PRIME_NEVER_ASSERTMSG(cond, ...) ((void)0)
#define PRIME_NEVER_GUARDMSG(cond, ...) (cond)

#define PRIME_ALWAYS_EXPECTMSG(cond, ...) ((cond) ? ((void)0) : Prime::AssertionFailed(__FILE__, __LINE__, #cond, __VA_ARGS__))
#define PRIME_NEVER_EXPECTMSG(cond, ...) ((void)(cond))
#endif

#if !defined(PRIME_FINAL)
#define PRIME_ASSERTMSG PRIME_ALWAYS_ASSERTMSG
#define PRIME_GUARDMSG PRIME_ALWAYS_GUARDMSG
#else
#define PRIME_ASSERTMSG PRIME_NEVER_ASSERTMSG
#define PRIME_GUARDMSG PRIME_NEVER_GUARDMSG
#endif

#define PRIME_EXPECTMSG PRIME_ALWAYS_EXPECTMSG

#if defined(PRIME_DEBUG)
#define PRIME_DEBUG_ASSERTMSG PRIME_ASSERTMSG
#define PRIME_DEBUG_GUARDMSG PRIME_GUARDMSG
#define PRIME_DEBUG_EXPECTMSG PRIME_EXPECTMSG
#else
#define PRIME_DEBUG_ASSERTMSG PRIME_NEVER_ASSERTMSG
#define PRIME_DEBUG_GUARDMSG PRIME_NEVER_GUARDMSG
#define PRIME_DEBUG_EXPECTMSG PRIME_NEVER_EXPECTMSG
#endif

#define PRIME_ALWAYS_ASSERT(cond) PRIME_ALWAYS_ASSERTMSG(cond, NULL)
#define PRIME_ALWAYS_GUARD(cond) PRIME_ALWAYS_GUARDMSG(cond, NULL)
#define PRIME_ALWAYS_EXPECT(cond) PRIME_ALWAYS_EXPECTMSG(cond, NULL)

#define PRIME_ASSERT(cond) PRIME_ASSERTMSG(cond, NULL)
#define PRIME_GUARD(cond) PRIME_GUARDMSG(cond, NULL)
#define PRIME_EXPECT(cond) PRIME_EXPECTMSG(cond, NULL)

#define PRIME_DEBUG_ASSERT(cond) PRIME_DEBUG_ASSERTMSG(cond, NULL)
#define PRIME_DEBUG_GUARD(cond) PRIME_DEBUG_GUARDMSG(cond, NULL)
#define PRIME_DEBUG_EXPECT(cond) PRIME_DEBUG_EXPECTMSG(cond, NULL)

//
// PRIME_TEST
// To distinguish tests from assertions.
//

#define PRIME_TEST PRIME_ALWAYS_ASSERT
#define PRIME_TESTMSG PRIME_ALWAYS_ASSERTMSG

//
// PRIME_UNREACHABLE / PRIME_TODO
//

#define PRIME_UNREACHABLE() Prime::AssertionFailed(__FILE__, __LINE__, "unreachable", NULL)
#define PRIME_TODO() Prime::AssertionFailed(__FILE__, __LINE__, "TODO", NULL)

//
// Developer Mode
//
// Enables an application to enable developer-only features. Defaults to true in Debug and Release builds under the
// assumption that until you've added a command line flag or similar you'll want it enabled.
//

PRIME_PUBLIC extern bool developerMode;
PRIME_PUBLIC extern int debuggerEnabled;
PRIME_PUBLIC extern bool checkedIfRunninginDebugger;

/// On platforms where there's no way to determine this, returns true in Debug builds, false in Final builds,
/// and the result of GetDeveloperMode() in Release builds.
PRIME_PUBLIC bool IsDebuggerAttached() PRIME_NOEXCEPT;

inline bool GetDeveloperMode() PRIME_NOEXCEPT
{
    if (!checkedIfRunninginDebugger) {
        if (IsDebuggerAttached()) {
            developerMode = 1;
        }
        checkedIfRunninginDebugger = true;
    }
    return developerMode;
}

PRIME_PUBLIC void SetDeveloperMode(bool newDeveloperMode) PRIME_NOEXCEPT;

//
// Developer Logging
// (In Prime, these are implemented in Log.cpp using the global Log object - if you're not using Log then you'll
// need to implement them manually.)
//

/// Use this for developer debug output (where you might otherwise use printf() or OutputDebugString()).
PRIME_PUBLIC void Trace(const char* format, ...);

/// Use this to report anything a developer should see in the logs (or on-screen if in developer mode).
PRIME_PUBLIC void DeveloperWarning(const char* format, ...);

/// Use this to report a runtime error which has left the application in a corrupt state. Unless overridden by a
/// developer, the application should probably terminate, but in developer mode the user may be able to resume.
PRIME_PUBLIC void RuntimeError(const char* format, ...);

#ifdef PRIME_NO_VA_ARGS
inline void DiscardLog(const char*, ...)
{
}
#endif

#if !defined(PRIME_FINAL)
#define PRIME_TRACE Prime::Trace
#define PRIME_DEVELOPER_WARNING Prime::DeveloperWarning
#define PRIME_RUNTIME_ERROR Prime::RuntimeError
#define PRIME_HERE() Prime::Trace("%s", __FUNCTION__)
#else
#ifdef PRIME_NO_VA_ARGS
#define PRIME_TRACE Prime::DiscardLog
#define PRIME_DEVELOPER_WARNING Prime::DiscardLog
#define PRIME_RUNTIME_ERROR Prime::DiscardLog
#else
#define PRIME_TRACE(...) ((void)0)
#define PRIME_DEVELOPER_WARNING(...) ((void)0)
#define PRIME_RUNTIME_ERROR(...) ((void)0)
#endif
#define PRIME_HERE() ((void)0)
#endif

//
// Spewing
// A .cpp can `#include "Spew.h"` to enable "spew logging" on a per-.cpp basis, then the #include's can be
// commented-out or #ifdef'd out to disable spewing. Without Spew.h/Spew2.h included, these macros are all no-ops.
// Spew logging should never be committed to version control, it provides a means for a module to dump a large amount
// of logging during development.
//

#ifdef PRIME_NO_VA_ARGS
#define PRIME_SPEW (void)
#define PRIME_IF_SPEW(x) ((void)0)

#define PRIME_SPEW2 (void)
#define PRIME_IF_SPEW2(x) ((void)0)
#else
#define PRIME_SPEW(...) ((void)0)
#define PRIME_IF_SPEW(x) ((void)0)

#define PRIME_SPEW2(...) ((void)0)
#define PRIME_IF_SPEW2(x) ((void)0)
#endif

//
// Debugger detection
//

/// Returns true if PRIME_DEBUGGER will have the desired effect. This returns the result of
/// IsDebuggerAttached, but can be overridden by SetDebuggerEnabled().
PRIME_PUBLIC bool IsDebuggerEnabled() PRIME_NOEXCEPT;

/// Override the value returned by IsDebuggerEnabled().
PRIME_PUBLIC void SetDebuggerEnabled(bool value) PRIME_NOEXCEPT;

/// A breakpoint only if we're running in a debugger (or SetDebuggerEnabled(true) has been called).
#define PRIME_DEBUGGER()                  \
    {                                     \
        if (Prime::IsDebuggerEnabled()) { \
            PRIME_ALWAYS_DEBUGGER();      \
        }                                 \
    }

//
// Templates we want/need to be ubiquitous
//

#ifndef PRIME_COMPILER_NO_PARTIAL_TEMPLATE_SPECIALISATION

template <typename T>
struct RemovePointer {
    typedef T type;
};
template <typename T>
struct RemovePointer<T*> {
    typedef T type;
};
template <typename T>
struct RemovePointer<T* const> {
    typedef T type;
};
template <typename T>
struct RemovePointer<T* volatile> {
    typedef T type;
};
template <typename T>
struct RemovePointer<T* const volatile> {
    typedef T type;
};

template <typename T>
struct RemoveReference {
    typedef T type;
};
template <typename T>
struct RemoveReference<T&> {
    typedef T type;
};

#endif

template <bool Condition, typename T>
struct EnableIf {
};
template <typename T>
struct EnableIf<true, T> {
    typedef T Type;
};

template <bool Condition, typename T>
struct EnableIfNot {
};
template <typename T>
struct EnableIfNot<false, T> {
    typedef T Type;
};

template <typename A, typename B>
struct IsSameType {
    PRIME_STATIC_CONST_BOOL(value, false);
};
template <typename Type>
struct IsSameType<Type, Type> {
    PRIME_STATIC_CONST_BOOL(value, true);
};

template <typename Type>
struct IsSigned {
public:
    PRIME_STATIC_CONST_BOOL(value, Type(-1) < Type(0));
};

template <class T, class U>
struct IsSameSignedness {

    PRIME_STATIC_CONST_BOOL(value, IsSigned<T>::value == IsSigned<U>::value);
};

/// Convert a From to a To and inassert the value hasn't been truncated.
template <typename To, typename From>
inline To Narrow(From from) PRIME_NOEXCEPT
{
    To to = static_cast<To>(from);
    PRIME_DEBUG_ASSERTMSG(static_cast<From>(to) == from && ((IsSameSignedness<To, From>::value) || (to < To()) == (from < From())), "Narrowing conversion error");
    return to;
}

/// Convert an integer From to an enum type To and assert that the value hasn't been truncated.
template <typename Enum, typename Integer>
inline Enum IntegerToEnum(Integer i) PRIME_NOEXCEPT
{
    return Narrow<Enum>(i);
}

/// Convert an enum type From to an integer To and assert that the value hasn't been truncated.
template <typename Integer, typename Enum>
inline Integer EnumToInteger(Enum e) PRIME_NOEXCEPT
{
    return Narrow<Integer>(e);
}

template <typename T>
inline T QuantiseAbs(T value)
{
    return value < T() ? -value : value;
}

/// Convert a From to a To and ensure it's within the representable range.
template <typename To, typename From>
inline To Quantise(From from)
{
    To to = static_cast<To>(from);
    PRIME_DEBUG_ASSERTMSG(QuantiseAbs(static_cast<From>(to) - from) < static_cast<From>(2) && ((IsSameSignedness<To, From>::value) || (to < To()) == (from < From())), "Quantise error");
    return to;
}

/// static_cast that uses RTTI to check the result in debug builds (therefore, From must have a vtable)
template <typename To, typename From>
inline To CheckedStaticCast(From* from)
{
#if !defined(PRIME_NO_RTTI)
    PRIME_DEBUG_ASSERT(dynamic_cast<To>(from) != NULL);
#endif

    return static_cast<To>(from);
}

/// static_cast that uses RTTI to check the result in debug builds (therefore, From must have a vtable)
template <typename To, typename From>
inline To CheckedStaticCast(From& from)
{
#if !defined(PRIME_NO_RTTI)
    PRIME_DEBUG_ASSERT(dynamic_cast<typename RemoveReference<To>::type*>(&from) != NULL);
#endif

    return static_cast<To>(from);
}

//
// Aligned memory allocation
//

PRIME_PUBLIC void* AllocateAligned(size_t size, size_t alignment);
PRIME_PUBLIC void FreeAligned(void* ptr);
PRIME_PUBLIC void* ReallocateAligned(void* ptr, size_t newSize, size_t newAlignment);

//
// Thread yielding
//

/// Call this if you're going to block the calling thread. Designed to be called by code which expects to be run
/// in a thread, so it isn't called by most library code. Returns true if the thread has successfully yielded
/// and should call ResumeThreadDoNotCallDirectly() when it wishes to continue. Due to the possibility of
/// forgetting to call ResumeThreadDoNotCallDirectly(), you should use ScopedYieldThread where possible.
PRIME_PUBLIC bool YieldThreadDoNotCallDirectly();

/// Tell the thread manager that the calling thread wishes to resume after a previous successful call to
/// YieldThreadDoNotCallDirectly(). Due to the possibility of forgetting to call ResumeThreadDoNotCallDirectly(),
/// you should use ScopedYieldThread where possible.
PRIME_PUBLIC void ResumeThreadDoNotCallDirectly();

/// A safer way to yield. Use RAII to ensure we resume if we yielded.
class ScopedYieldThread {
public:
    ScopedYieldThread()
    {
        _yielded = YieldThreadDoNotCallDirectly();
    }

    explicit ScopedYieldThread(bool yieldNow)
    {
        _yielded = yieldNow && YieldThreadDoNotCallDirectly();
    }

    ~ScopedYieldThread()
    {
        if (_yielded) {
            ResumeThreadDoNotCallDirectly();
        }
    }

    void yield()
    {
        resume();
        _yielded = YieldThreadDoNotCallDirectly();
    }

    void resume()
    {
        if (_yielded) {
            ResumeThreadDoNotCallDirectly();
            _yielded = false;
        }
    }

private:
    bool _yielded;

    PRIME_UNCOPYABLE(ScopedYieldThread);
};

//
// Localisation
//

/// Provides a gettext style service for providing a translated version of an English string.
#define PRIME_LOCALISE(english) Prime::GetLocalised("" english "", NULL)

/// Provides a gettext style service for providing a translated version of an English string.
#define PRIME_LOCALISE2(english, description) Prime::GetLocalised("" english "", "" description "")

PRIME_PUBLIC const char* GetLocalised(const char* english, const char* description) PRIME_NOEXCEPT;

//
// Safe string copy/append
//

/// Safe replacement for strcpy. Never overruns the buffer and guarantees null termination.
PRIME_PUBLIC bool StringCopy(char* buffer, size_t bufferSize, const char* src) PRIME_NOEXCEPT;
PRIME_PUBLIC bool StringCopy(char* buffer, size_t bufferSize, const char* src, size_t n) PRIME_NOEXCEPT;

#if !defined(PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES)

template <size_t bufferSize>
bool StringCopy(char (&buffer)[bufferSize], const char* src) PRIME_NOEXCEPT
{
    return StringCopy(buffer, bufferSize, src);
}

template <size_t bufferSize>
bool StringCopy(char (&buffer)[bufferSize], const char* src, size_t n) PRIME_NOEXCEPT
{
    return StringCopy(buffer, bufferSize, src, n);
}

#endif

/// Safe replacement for strcat. Guarantees null termination.
PRIME_PUBLIC bool StringAppend(char* buffer, size_t bufferSize, const char* src) PRIME_NOEXCEPT;
PRIME_PUBLIC bool StringAppend(char* buffer, size_t bufferSize, const char* src, size_t n) PRIME_NOEXCEPT;

#if !defined(PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES)

template <size_t bufferSize>
bool StringAppend(char (&buffer)[bufferSize], const char* src) PRIME_NOEXCEPT
{
    return StringAppend(buffer, bufferSize, src);
}

template <size_t bufferSize>
bool StringAppend(char (&buffer)[bufferSize], const char* src, size_t n) PRIME_NOEXCEPT
{
    return StringAppend(buffer, bufferSize, src, n);
}

#endif

//
// Safe string formatting
//

/// Safe replacement for sprintf. Guarantees null termination if bufferSize > 0.
PRIME_PUBLIC bool StringFormat(char* buffer, size_t bufferSize, const char* format, ...) PRIME_NOEXCEPT;

/// Safe replacement for vsprintf. Guarantees null termination if bufferSize > 0.
PRIME_PUBLIC bool StringFormatVA(char* buffer, size_t bufferSize, const char* format, va_list argptr) PRIME_NOEXCEPT;

#if !defined(PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES)

template <size_t bufferSize>
bool StringFormat(char (&buffer)[bufferSize], const char* format, ...) PRIME_NOEXCEPT
{
    va_list argptr;
    va_start(argptr, format);
    bool result = StringFormatVA(buffer, bufferSize, format, argptr);
    va_end(argptr);
    return result;
}

template <size_t bufferSize>
bool StringFormatVA(char (&buffer)[bufferSize], const char* format, va_list argptr) PRIME_NOEXCEPT
{
    return StringFormatVA(buffer, bufferSize, format, argptr);
}

#endif

/// Like StringFormat, but sets length to the (truncated) length of the output string.
PRIME_PUBLIC bool StringFormatLength(size_t& length, char* buffer, size_t bufferSize, const char* format, ...) PRIME_NOEXCEPT;
PRIME_PUBLIC bool StringFormatLengthVA(size_t& length, char* buffer, size_t bufferSize, const char* format, va_list argptr) PRIME_NOEXCEPT;

#if !defined(PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES)

template <size_t bufferSize>
bool StringFormatLength(size_t& length, char (&buffer)[bufferSize], const char* format, ...) PRIME_NOEXCEPT
{
    va_list argptr;
    va_start(argptr, format);
    bool result = StringFormatLengthVA(length, buffer, bufferSize, format, argptr);
    va_end(argptr);
    return result;
}

template <size_t bufferSize>
bool StringFormatLengthVA(size_t& length, char (&buffer)[bufferSize], const char* format, va_list argptr) PRIME_NOEXCEPT
{
    return StringFormatLengthVA(length, buffer, bufferSize, format, argptr);
}

#endif

/// A safe alternative to sprintf(str + strlen(str), ...). Never overruns the buffer and guarantees null
/// termination.
PRIME_PUBLIC bool StringAppendFormat(char* buffer, size_t bufferSize, const char* format, ...) PRIME_NOEXCEPT;
PRIME_PUBLIC bool StringAppendFormatVA(char* buffer, size_t bufferSize, const char* format, va_list argptr) PRIME_NOEXCEPT;

#ifndef PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES

template <size_t bufferSize>
bool StringAppendFormat(char (&buffer)[bufferSize], const char* format, ...) PRIME_NOEXCEPT
{
    va_list argptr;
    va_start(argptr, format);
    bool result = StringAppendFormatVA(buffer, bufferSize, format, argptr);
    va_end(argptr);
    return result;
}

template <size_t bufferSize>
bool StringAppendFormatVA(char (&buffer)[bufferSize], const char* format, va_list argptr) PRIME_NOEXCEPT
{
    return StringAppendFormatVA(buffer, bufferSize, format, argptr);
}

#endif

//
// Global
//

/// Wrap a type so that construction registers the object as the global object for that type.
/// Requires the Base type has setGlobal and getGlobal static member functions.
/// Example: Global<DefaultLog> log;
template <typename Base>
class Global : public Base {
public:
    explicit Global(bool becomeGlobal = true)
    {
        if (becomeGlobal) {
            Base::setGlobal(this);
        }
    }

    explicit Global(const char* applicationNameOrArgv0, bool becomeGlobal = true)
    {
        if (applicationNameOrArgv0) {
            static_cast<Base*>(this)->setApplicationName(applicationNameOrArgv0);
        }

        if (becomeGlobal) {
            Base::setGlobal(this);
        }
    }

    ~Global()
    {
        if (Base::getGlobal() == this) {
            Base::setGlobal(NULL);
        }
    }
};

}

#endif
