// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWSCONFIG_H
#define PRIME_WINDOWSCONFIG_H

// Don't add any of the Windows header defines (STRICT, WIN32_LEAN_AND_MEAN, etc.) here, put them in Config.h.

#include "../Config.h"

//
// Include the Windows' header file (windows.h, xtl.h or nothing) depending on the platform detected by Config.h.
//

#if defined(PRIME_OS_XBOX)
#include <xtl.h>
#elif defined(PRIME_OS_WINDOWS)
#include <windows.h>
#endif

#include "../StringView.h"
#include <string>

#ifdef __cplusplus
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif
#endif

#ifndef PRIME_OS_WINDOWS
#define TEXT(text) text
#endif

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

//
// Conversions between UTF-8 and WCHAR, allowing char strings to be used to pass Windows file names around.
//
// e.g.:
//
//     if (! ::DeleteFile(CharToTChar(path).c_str()))
//         ...
//
// On non-Unicode versions of Windows, the PRIME_WIN98_UTF8 define (in Config.h) decides whether UTF-8 is used or not.
//

namespace Prime {

#if defined(PRIME_OS_WINDOWS)

typedef std::basic_string<TCHAR> TCharString;
typedef std::basic_string<WCHAR> WCharString;

typedef BasicStringView<TCHAR> TCharStringView;
typedef BasicStringView<WCHAR> WCharStringView;

PRIME_PUBLIC std::string WCharToChar(UINT codepage, WCharStringView wstring);
PRIME_PUBLIC WCharString CharToWChar(UINT codepage, StringView string);

PRIME_PUBLIC std::string TCharToChar(TCharStringView string);
PRIME_PUBLIC TCharString CharToTChar(StringView string);

#endif
}

#endif
