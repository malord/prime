// Copyright 2000-2021 Mark H. P. Lord

#include "Common.h"
#include "StringUtils.h"
#include <algorithm>
#include <stdio.h>
#include <string.h>

//
// Developer mode
//

namespace Prime {
bool developerMode = false;
bool checkedIfRunninginDebugger = false;
}

//
// Debugger detection
//

namespace Prime {
int debuggerEnabled = -1;
}

namespace Prime {

void SetDeveloperMode(bool newDeveloperMode) PRIME_NOEXCEPT
{
    developerMode = newDeveloperMode;
    checkedIfRunninginDebugger = true;
}

bool IsDebuggerEnabled() PRIME_NOEXCEPT
{
    if (debuggerEnabled >= 0) {
        return debuggerEnabled != 0;
    }

    return IsDebuggerAttached();
}

void SetDebuggerEnabled(bool value) PRIME_NOEXCEPT
{
    debuggerEnabled = value ? 1 : 0;
}
}

// 360 has DmIsDebuggerPresent(), but that needs to be stripped from final builds.
#if defined(PRIME_OS_WINDOWS) && !defined(PRIME_OS_XBOX) && !PRIME_MSC_AND_OLDER(1300)

#include "Windows/WindowsConfig.h"

namespace Prime {

bool IsDebuggerAttached() PRIME_NOEXCEPT
{
    return IsDebuggerPresent() ? true : false;
}
}

#elif defined(PRIME_OS_MAC) || (defined(PRIME_OS_IOS) && !defined(PRIME_FINAL))

#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

namespace Prime {

bool IsDebuggerAttached() PRIME_NOEXCEPT
{
    int mib[4];
    struct kinfo_proc info;
    size_t size;

    info.kp_proc.p_flag = 0;
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    size = sizeof(info);
    sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);

    return ((info.kp_proc.p_flag & P_TRACED) != 0);
}
}

#else

namespace Prime {

bool IsDebuggerAttached() PRIME_NOEXCEPT
{
#ifdef PRIME_FINAL
    return false;
#elif defined(PRIME_DEBUG)
    return developerMode;
#else
    return false;
#endif
}
}

#endif

//
// Localisation
//

namespace Prime {

const char* GetLocalised(const char* english, const char*) PRIME_NOEXCEPT
{
    // Not needed this yet
    return english;
}
}

//
// Safe string functions
//

namespace Prime {

//
// Safe string copy/append
//

bool StringCopy(char* buffer, size_t bufferSize, const char* src) PRIME_NOEXCEPT
{
    char* terminator;

    if (!PRIME_GUARD(bufferSize)) {
        return false;
    }

    terminator = buffer + bufferSize - 1;

    while (*src) {
        if (buffer == terminator) {
            *buffer = 0;
            return false;
        }

        *buffer++ = *src++;
    }

    *buffer = 0;
    return true;
}

bool StringCopy(char* buffer, size_t bufferSize, const char* src, size_t n) PRIME_NOEXCEPT
{
    char* terminator;

    if (!PRIME_GUARD(bufferSize)) {
        return false;
    }

    terminator = buffer + bufferSize - 1;

    while (*src && n--) {
        if (buffer == terminator) {
            *buffer = 0;
            return false;
        }

        *buffer++ = *src++;
    }

    *buffer = 0;
    return true;
}

bool StringAppend(char* buffer, size_t bufferSize, const char* src) PRIME_NOEXCEPT
{
    size_t length = strlen(buffer);

    if (length >= bufferSize) {
        return false;
    }

    return StringCopy(buffer + length, bufferSize - length, src);
}

bool StringAppend(char* buffer, size_t bufferSize, const char* src, size_t n) PRIME_NOEXCEPT
{
    size_t length = strlen(buffer);

    if (length >= bufferSize) {
        return false;
    }

    return StringCopy(buffer + length, bufferSize - length, src, n);
}

//
// Safe string formatting
//

#if defined(_MSC_VER)
#define PRIME_MSC_VSNPRINTF
#elif defined(__BORLANDC__) || defined(__TURBOC__)
#if __TURBOC__ >= 0x500
#define PRIME_MSC_VSNPRINTF
#endif
#else
#define PRIME_GOOD_SNPRINTF
#endif

bool StringFormatLengthVA(size_t& length, char* buffer, size_t bufferSize, const char* format, va_list argptr) PRIME_NOEXCEPT
{
#if defined(PRIME_GOOD_SNPRINTF) || defined(PRIME_MSC_VSNPRINTF)

    if (!PRIME_GUARD(bufferSize)) {
        length = 0;

        return false;
    }

#ifdef PRIME_GOOD_SNPRINTF
    ptrdiff_t result = vsnprintf(buffer, bufferSize, format, argptr);
#elif defined(PRIME_MSC_VSNPRINTF)
    ptrdiff_t result = _vsnprintf(buffer, bufferSize - 1, format, argptr);
#endif

    if (result < 0) {
        buffer[0] = 0;
        length = 0;

        return false;
    }

    if ((size_t)result >= bufferSize) {
        buffer[bufferSize - 1] = 0;
        length = bufferSize - 1;
        return false;
    }

    length = (size_t)result;
    return true;

#else

    // For platforms that don't have any kind of vsnprintf, print to a big(ish) memory buffer and hope for the best.

    size_t dynamicBufferSize = 32768u;

    if (!PRIME_GUARD(bufferSize)) {
        return false;
    }

    while (dynamicBufferSize > bufferSize) {
        char* dynamicBuffer = new char[dynamicBufferSize];
        if (!dynamicBuffer) {
            dynamicBufferSize /= 2;
            continue;
        }

        va_list argptr2;
        PRIME_VA_COPY(argptr2, argptr);
        vsprintf(dynamicBuffer, format, argptr2);
        va_end(argptr2);

        size_t len = strlen(dynamicBuffer);
        if (len >= bufferSize) {
            if (len >= dynamicBufferSize) {
                RuntimeError("vsprintf overflow");
            }

            memcpy(buffer, dynamicBuffer, bufferSize - 1);
            buffer[bufferSize - 1] = 0;
            delete[] dynamicBuffer;
            length = len;
            return false;
        }

        memcpy(buffer, dynamicBuffer, len + 1);
        delete[] dynamicBuffer;
        length = len;
        return true;
    }

    vsprintf(buffer, format, argptr);

    size_t len = strlen(buffer);
    if (len >= bufferSize) {
        RuntimeError("vsprintf overflow");
    }

    length = len;
    return true;

#endif
}

bool StringFormatVA(char* buffer, size_t bufferSize, const char* format, va_list argptr) PRIME_NOEXCEPT
{
    size_t unusedLength;
    return StringFormatLengthVA(unusedLength, buffer, bufferSize, format, argptr);
}

bool StringFormat(char* buffer, size_t bufferSize, const char* format, ...) PRIME_NOEXCEPT
{
    bool result;
    va_list argptr;
    va_start(argptr, format);
    result = StringFormatVA(buffer, bufferSize, format, argptr);
    va_end(argptr);
    return result;
}

bool StringFormatLength(size_t& length, char* buffer, size_t bufferSize, const char* format, ...) PRIME_NOEXCEPT
{
    bool result;
    va_list argptr;
    va_start(argptr, format);
    result = StringFormatLengthVA(length, buffer, bufferSize, format, argptr);
    va_end(argptr);
    return result;
}

bool StringAppendFormat(char* buffer, size_t bufferSize, const char* format, ...) PRIME_NOEXCEPT
{
    bool result;
    va_list argptr;
    va_start(argptr, format);
    result = StringAppendFormatVA(buffer, bufferSize, format, argptr);
    va_end(argptr);
    return result;
}

bool StringAppendFormatVA(char* buffer, size_t bufferSize, const char* format, va_list argptr) PRIME_NOEXCEPT
{
    size_t length = strlen(buffer);

    if (length >= bufferSize) {
        return false;
    }

    return StringFormatVA(buffer + length, bufferSize - length, format, argptr);
}
}
