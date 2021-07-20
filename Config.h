// Copyright 2000-2021 Mark H. P. Lord

// Perform Prime specific configuration, then include Common.h (which provides assertions, developer logging,
// memory functions, etc.). If you don't have zlib, win-iconv or miniconv then you need to edit this file.

#ifndef PRIME_CONFIG_H
#define PRIME_CONFIG_H

#ifdef _MSC_VER
// Disable MSC's warnings about deprecated C library functions.
#pragma warning(disable : 4996)

// "class X needs to have dll-interface to be used by clients of class Y"
#pragma warning(disable : 4251)
#endif

// Use Platform.h to detect as much as possible about the target
#include "Platform.h"

// Provide per-repository configuration in PrimeConfig.h
#ifdef PRIMECONFIG_HERE
/// #define PRIMECONFIG_HERE if you don't want it littering the directory above Prime.
#include "PrimeConfig.h"
#else
// For easier user in git submodules, look for PrimeConfig.h in the directory above Prime.
#include "../PrimeConfig.h"
#endif

//
// Default configuration options if PrimeConfig.h hasn't supplied them
//

#ifndef PRIME_WINVER
#if PRIME_MSC_AND_OLDER(1300)
#define PRIME_WINVER PRIME_WINDOWS_95
#else
#define PRIME_WINVER PRIME_WINDOWS_VISTA
#endif
#endif

// (Windows has win-iconv, a lightweight wrapper around Windows' character set conversion APIs.)
#if !defined(PRIME_OS_UNIX) && !(defined(PRIME_OS_WINDOWS) && !defined(PRIME_OS_XBOX))
// Platform doesn't have iconv, so disable by default
#define PRIME_NO_ICONV
#endif

// Disable miniconv if we have iconv.
#if !defined(PRIME_NO_ICONV)
// If iconv is available then by default, don't require miniconv
#define PRIME_NO_MINICONV
#endif

//
// Don't edit anything below this line
//

// Define PRIME_PUBLIC, used to export symbols in a DLL/.dylib/.so
#if defined(PRIME_OS_WINDOWS)
#if defined(PRIME_DLL_EXPORT)
#define PRIME_PUBLIC PRIME_EXPORT
#elif defined(PRIME_DLL) || defined(_USE_DLLS)
#define PRIME_PUBLIC PRIME_IMPORT
#else
#define PRIME_PUBLIC
#endif
#else
#define PRIME_PUBLIC PRIME_EXPORT
#endif

// Usually you won't want to change any of this and can just #define PRIME_WINVER in PrimeConfig.h.
#if defined(PRIME_OS_WINDOWS) && !defined(PRIME_NO_WINDOWS_CONFIG)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef STRICT
#define STRICT
#endif

#if defined(WINVER) || defined(_WIN32_WINDOWS) || defined(_WIN32_WINNT)
#error "Either include Config.h before any other headers, or define PRIME_NO_WINDOWS_CONFIG"
#else
#define WINVER PRIME_WINVER_FOR(PRIME_WINVER)
#define _WIN32_WINDOWS PRIME_WIN32_WINDOWS_FOR(PRIME_WINVER)
#define _WIN32_WINNT PRIME_WIN32_WINNT_FOR(PRIME_WINVER)

// Don't override _WIN32_IE, to allow, e.g., Win98 with IE5
#if !defined(_WIN32_IE)
#define _WIN32_IE PRIME_WIN32_IE_FOR(PRIME_WINVER)
#endif
#endif

#endif

#include "Common.h"

#endif
