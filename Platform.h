// Copyright 2000-2021 Mark H. P. Lord

//
// Platform.h
// Copyright (c) 2002-2020 Mark H. P. Lord. MIT Licensed (see LICENSE.txt).
//
// Compiler and platform detection and abstraction, including C99 integer types.
//
// Regularly tested platforms:
//   Windows, macOS, iOS, Android, Linux
//
// Once known to have worked on:
//   PS3 (PPU/SPU), 360, PSP, PS Vita, Wii, FreeBSD, DOS, DOS-32, BB10, Mac PPC
//
// Config:
//   #define PRIME_NO_C99_INTTYPES to prevent C99 strict integer types being defined.
//

#ifndef PRIME_PLATFORM_H
#define PRIME_PLATFORM_H

#if defined(__cplusplus) && !defined(PRIME_PLATFORM_NO_CXX)
#define PRIME_CXX __cplusplus
#endif

//
// Compiler detection and warning overrides
//

#ifdef _MSC_VER
#define PRIME_COMPILER_NAME "MSC"

// Because I keep forgetting:
// MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)
// MSVC++ 11.0 _MSC_VER == 1700 (Visual Studio 2012)
// MSVC++ 10.0 _MSC_VER == 1600 (Visual Studio 2010)
// MSVC++ 9.0  _MSC_VER == 1500 (Visual Studio 2008)
// MSVC++ 8.0  _MSC_VER == 1400 (Visual Studio 2005)
// MSVC++ 7.1  _MSC_VER == 1310 (Visual Studio 2003)
// MSVC++ 7.0  _MSC_VER == 1300
// MSVC++ 6.0  _MSC_VER == 1200
// MSVC++ 5.0  _MSC_VER == 1100

#if _MSC_VER < 1300
#define PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES
#define PRIME_COMPILER_NO_MEMBER_TEMPLATE_OVERRIDES
#define PRIME_COMPILER_NO_STATIC_CONST
#define PRIME_COMPILER_NO_REVERSE_ITER_FROM_FORWARD_ITER
#define PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
#define PRIME_COMPILER_NO_REVERSE_ITERATOR
#define PRIME_COMPILER_NO_PARTIAL_TEMPLATE_SPECIALISATION

#pragma warning(disable : 4284)
#pragma warning(disable : 4786)
#pragma warning(disable : 4511) // couldn't generate copy ctor
#pragma warning(disable : 4512) // couldn't generate assignment operator
#pragma warning(disable : 4514) // unreferenced inline function removed
#pragma warning(disable : 4097) // typedef-name used as synonym
#pragma warning(disable : 4786) // symbol truncated to 255 chars in debug information
#pragma warning(disable : 4503) // decorated name length exceeded, name was truncated

// These are not warnings I want to disable, but the VC6 standard library triggers them
#pragma warning(disable : 4100) // unreferenced parameter
#pragma warning(disable : 4663) // C++ language change
#pragma warning(disable : 4245) // signed/unsigned mismatch while converting
#pragma warning(disable : 4018) // signed/unsigned mismatch in operator
#endif

#elif defined(__clang__)
#define PRIME_COMPILER_NAME "Clang"

#elif defined(__BORLANDC__)
#define PRIME_COMPILER_NAME "Borland"

#elif defined(__TURBOC__)
#define PRIME_COMPILER_NAME "Turbo"

#elif defined(__WATCOMC__)
#define PRIME_COMPILER_NAME "Watcom"

#elif defined(__SNC__)
#define PRIME_COMPILER_NAME "SNC"

#elif defined(__GNUC__)
#define PRIME_COMPILER_NAME "GCC"

#endif

#ifndef PRIME_COMPILER_NAME
#error Unknown compiler. Please update Platform.h
#endif

#define PRIME_GCC_AND_NEWER(a, b, c) (defined(__GNUC__) && (__GNUC__ > a || (__GNUC__ == a && (__GNUC_MINOR__ > b || (__GNUC_MINOR__ == b && __GNUC_PATCHLEVEL__ >= c)))))
#define PRIME_GCC_AND_OLDER(a, b, c) (defined(__GNUC__) && (__GNUC__ < a || (__GNUC__ == a && (__GNUC_MINOR__ < b || (__GNUC_MINOR__ == b && __GNUC_PATCHLEVEL__ < c)))))

#if defined(_MSC_VER) // Testing for defined(_MSC_VER) within the macro fails in VC6
#define PRIME_MSC_AND_NEWER(n) (_MSC_VER >= n)
#define PRIME_MSC_AND_OLDER(n) (_MSC_VER < n)
#else
#define PRIME_MSC_AND_NEWER(n) 0
#define PRIME_MSC_AND_OLDER(n) 0
#endif

#if !defined(__clang__) || !defined(__has_builtin)
#define PRIME_CLANG_HAS_BUILTIN(name) 0
#else
#define PRIME_CLANG_HAS_BUILTIN(name) __has_builtin(name)
#endif

#if !defined(__clang__) || !defined(__has_feature)
#define PRIME_CLANG_HAS_FEATURE(name) 0
#else
#define PRIME_CLANG_HAS_FEATURE(name) __has_feature(name)
#endif

#if !defined(__clang__) || !defined(__has_extension)
#define PRIME_CLANG_HAS_EXTENSION(name) 0
#else
#define PRIME_CLANG_HAS_EXTENSION(name) __has_extension(name)
#endif

#define PRIME_CLANG_HAS(name) (PRIME_CLANG_HAS_EXTENSION(name) || PRIME_CLANG_HAS_FEATURE(name))

#if PRIME_GCC_AND_OLDER(5, 0, 0)
#define PRIME_COMPILER_NO_CONST_ITER_CAST
#endif

//
// Check for C++11 feature availability
//

#if (defined(PRIME_CXX) && PRIME_CXX >= 201402L)

// Compiler in C++14 mode
#define PRIME_CXX11
#define PRIME_CXX14

// Feature defines occur later

#elif (defined(PRIME_CXX) && PRIME_CXX > 199711L)

// Compiler in C++11 mode
#define PRIME_CXX11

// Feature defines occur later

#elif defined(__clang__)

// #if PRIME_CLANG_HAS(cxx_rvalue_references)
// __has_feature only returns true for cxx_rvalue_references if we're compiling as C++0x source, but
// __has_extension returns true if the compiler is recent enough. Unfortunately, even if the compiler supports
// rvalue references, the standard library may not (lacking std::move). So, for now, disable rvalue references.
#if PRIME_CLANG_HAS_FEATURE(cxx_rvalue_references)
#define PRIME_COMPILER_RVALUEREF
#endif

#if PRIME_CLANG_HAS_FEATURE(cxx_generalized_initializers)
#define PRIME_COMPILER_INITLIST
#endif

#if PRIME_CLANG_HAS_FEATURE(cxx_override_control)
#define PRIME_OVERRIDE override
#endif

#if PRIME_CLANG_HAS_FEATURE(cxx_noexcept)
#define PRIME_NOEXCEPT noexcept
#define PRIME_NOEXCEPT_IF(x) noexcept(x)
#endif

#if PRIME_CLANG_HAS_FEATURE(cxx_static_assert)
#define PRIME_COMPILER_STATIC_ASSERT
#endif

#if PRIME_CLANG_HAS_FEATURE(cxx_variadic_templates)
#define PRIME_COMPILER_VARIADIC_TEMPLATES
#endif

#elif defined(_MSC_VER)

// Visual C++ doesn't define __cplusplus > 199711L as of Visual C++ 2015, but it's compatible
#if _MSC_VER >= 1900
#define PRIME_CXX11
#endif

#if _MSC_VER >= 1910
#define PRIME_CXX14
#endif

#if _MSC_VER >= 1600
#define PRIME_COMPILER_RVALUEREF
#define PRIME_COMPILER_INITLIST
#define PRIME_COMPILER_STATIC_ASSERT
#define PRIME_COMPILER_CXX11_ATOMICS
#define PRIME_COMPILER_NULLPTR
#define PRIME_COMPILER_EXPLICIT_CONVERSION
#define PRIME_CXX11_STL
#define PRIME_OVERRIDE override
#define PRIME_NOEXCEPT noexcept
#define PRIME_NOEXCEPT_IF(x) noexcept(x)
#endif

#if _MSC_VER >= 1800
#define PRIME_COMPILER_VARIADIC_TEMPLATES
#endif

#elif PRIME_GCC_AND_NEWER(4, 3, 0) && __GXX_EXPERIMENTAL_CXX0X__

#define PRIME_COMPILER_RVALUEREF
#define PRIME_COMPILER_INITLIST
#define PRIME_COMPILER_STATIC_ASSERT
#define PRIME_OVERRIDE override
#define PRIME_NOEXCEPT noexcept
#define PRIME_NOEXCEPT_IF(x) noexcept(x)
#define PRIME_COMPILER_VARIADIC_TEMPLATES

#endif

#ifdef PRIME_CXX11
#define PRIME_COMPILER_RVALUEREF
#define PRIME_COMPILER_INITLIST
#define PRIME_COMPILER_STATIC_ASSERT
#define PRIME_COMPILER_CXX11_ATOMICS
#define PRIME_COMPILER_NULLPTR
#define PRIME_CXX11_STL
#define PRIME_OVERRIDE override
#define PRIME_NOEXCEPT noexcept
#define PRIME_NOEXCEPT_IF(x) noexcept(x)
#define PRIME_COMPILER_VARIADIC_TEMPLATES
#define PRIME_COMPILER_OBJECTS_IN_UNIONS
#define PRIME_COMPILER_EXPLICIT_CONVERSION
#endif

// PRIME_MOVE expands to std::move if available, or expr if not.
#ifdef PRIME_COMPILER_RVALUEREF
#define PRIME_MOVE(expr) std::move(expr)
#define PRIME_MOVEITER(expr) std::make_move_iterator(expr)
#define PRIME_FORWARD(Type, expr) std::forward<Type>(expr)
#define PRIME_FORWARDING_REFERENCE(Type) Type&&
#else
#define PRIME_MOVE(expr) expr
#define PRIME_MOVEITER(expr) expr
#define PRIME_FORWARD(Type, expr) expr
#define PRIME_FORWARDING_REFERENCE(Type) const Type&
#endif

#ifndef PRIME_OVERRIDE
#define PRIME_OVERRIDE
#endif

#ifndef PRIME_NOEXCEPT
#define PRIME_NOEXCEPT throw()
#endif

#ifndef PRIME_NOEXCEPT_IF
#define PRIME_NOEXCEPT_IF(x)
#endif

#ifndef PRIME_COMPILER_NO_STATIC_CONST
#define PRIME_STATIC_CONST_BOOL(name, value) static const bool name = (value)
#define PRIME_STATIC_CONST(type, name, value) static const type name = (value)
#else
#define PRIME_STATIC_CONST_BOOL(name, value) enum { name = (value) ? 1 : 0 }
#define PRIME_STATIC_CONST(type, name, value) enum { name = static_cast<type>((value)) }
#endif

//
// Request C99 strict types in pre-C++11 C++ compilers.
//

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS 1
#endif

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

//
// Include the minimum set of system headers we need to get types/limits.
//

#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

//
// Compile time assertions
//

#ifdef PRIME_CXX
#ifdef PRIME_COMPILER_STATIC_ASSERT
#define PRIME_COMPILE_TIME_ASSERT(condition) static_assert((condition), #condition)
#else
#if !PRIME_MSC_AND_OLDER(1300)
namespace Prime {
template <bool Condition>
struct CompileTimeAssert {
};
template <>
struct CompileTimeAssert<true> {
    enum { CompileTimeAssertFailed = 1 };
    char unused;
};
template <>
struct CompileTimeAssert<false> { /* CompileTimeAssertFailed intentionally missing */
    char unused;
};
}

#define PRIME_COMPILE_TIME_ASSERT(condition) typedef char PrimeCompileTimeAssertTest[Prime::CompileTimeAssert<(condition)>::CompileTimeAssertFailed]
#else
// Old way - before C99 variable length arrays made it in to C++ compilers.
#define PRIME_COMPILE_TIME_ASSERT(condition) typedef char Compile_Time_Assertion_Failure[(condition) ? 1 : -1]
#endif
#endif
#else
#define PRIME_COMPILE_TIME_ASSERT(condition) extern char Compile_Time_Assertion_Failure[(condition) ? 1 : -1]
#endif

//
// Platform detection
//

#if (defined(__APPLE__) && defined(__MACH__)) || defined(PRIME_OS_IOS) || defined(PRIME_OS_MAC)

#if (defined(PRIME_CXX) && PRIME_CXX <= 199711L) && !defined(PRIME_DISABLE_OSX_CXX_WARNING)
#warning "macOS build not C++11"
#endif

// This is the only way I can find to detect Mac vs iOS vs simulator (define PRIME_OS_IOS if it isn't working).
#if !defined(PRIME_OS_IOS) && !defined(PRIME_OS_IOS_SIMULATOR) && !defined(PRIME_OS_MAC)
#include <TargetConditionals.h>
#endif

#if defined(PRIME_OS_IOS) || defined(PRIME_OS_IOS_SIMULATOR) || TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

#define PRIME_OS_NAME "iOS"
#define PRIME_OS_IOS
#define PRIME_OS_TOUCH

#if defined(PRIME_OS_IOS_SIMULATOR) || TARGET_IPHONE_SIMULATOR

#undef PRIME_OS_NAME
#define PRIME_OS_NAME "iOS Simulator"
#define PRIME_OS_IOS_SIMULATOR

#endif

#else

#define PRIME_OS_NAME "macOS"
#define PRIME_OS_MAC

#endif

#define PRIME_OS_OSX
#define PRIME_OS_DARWIN
#define PRIME_OS_UNIX
#define PRIME_OS_BSD
#define PRIME_OS_HAS_POSIX_SPAWN
#define PRIME_OS_HAS_VFORK

#elif defined(_XBOX_VER)

#if _XBOX_VER >= 200 && _XBOX_VER < 300

#define PRIME_OS_NAME "Xbox 360"
#define PRIME_OS_360
#define PRIME_OS_XBOX
#define PRIME_OS_WIN32
#define PRIME_OS_WINDOWS

#elif _XBOX_VER < 200

#define PRIME_OS_NAME "Xbox"
#define PRIME_OS_XBOX1
#define PRIME_OS_XBOX
#define PRIME_OS_WIN32
#define PRIME_OS_WINDOWS

#else

#error Unknown Xbox!

#endif

#if defined(UNICODE) || defined(_UNICODE)
#define PRIME_OS_WINDOWS_UNICODE
#endif

#elif defined(_WIN64) || defined(PRIME_OS_WIN64) || defined(_WIN32) || defined(PRIME_OS_WIN32)

#if defined(_WIN64) || defined(PRIME_OS_WIN64)

#define PRIME_OS_NAME "Win64"
#define PRIME_OS_WIN64

#else

#define PRIME_OS_NAME "Win32"
#define PRIME_OS_WIN32

#endif

#define PRIME_OS_WINDOWS

#if defined(CONSOLE) || defined(_CONSOLE)
#define PRIME_OS_WINDOWS_CONSOLE
#endif

#if defined(UNICODE) || defined(_UNICODE)
#define PRIME_OS_WINDOWS_UNICODE
#endif

#elif defined(SN_TARGET_PS3) || defined(__CELLOS_LV2__) || defined(__PS3_GCC_REVISION__) || defined(PRIME_OS_PS3)

#define PRIME_OS_PS3

#if defined(__PPU__)

#define PRIME_OS_NAME "PS3"
#define PRIME_OS_PS3_PPU

#elif defined(__SPU__)

#define PRIME_OS_NAME "PS3-SPU"
#define PRIME_OS_PS3_SPU

#else

#error Either __PPU__ or __SPU__ should be defined.

#endif

#elif defined(__ANDROID__) || defined(PRIME_OS_ANDROID)

#define PRIME_OS_NAME "Android"
#define PRIME_OS_ANDROID
#define PRIME_OS_UNIX
#define PRIME_OS_LINUX
//#define PRIME_OS_HAS_POSIX_SPAWN
#define PRIME_OS_HAS_VFORK

#elif defined(__linux__) || defined(PRIME_OS_LINUX)

#define PRIME_OS_NAME "Linux"
#define PRIME_OS_UNIX
#define PRIME_OS_LINUX
#define PRIME_OS_HAS_POSIX_SPAWN
#define PRIME_OS_HAS_VFORK

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonflyBSD__)

#define PRIME_OS_NAME "BSD"
#define PRIME_OS_UNIX
#define PRIME_OS_BSD
#define PRIME_OS_HAS_POSIX_SPAWN
#define PRIME_OS_HAS_VFORK

#elif defined(PRIME_OS_UNIX)

#define PRIME_OS_NAME "UNIX"
#define PRIME_OS_UNIX

#elif defined(__MSDOS__) || defined(__DOS__) || defined(_DOS) || defined(MSDOS) || defined(PRIME_OS_DOS32) || defined(PRIME_OS_DOS16)

#define PRIME_OS_DOS

#if defined(__386__) || defined(PRIME_OS_DOS32)

#define PRIME_OS_NAME "DOS-32"
#define PRIME_OS_DOS32
#define PRIME_CPU_386

#else

#define PRIME_OS_NAME "DOS"
#define PRIME_OS_DOS16
#define PRIME_CPU_X86_16

#endif

#elif defined(SN_TARGET_PSP2) || defined(__psp2__) || defined(PRIME_OS_PSP2)

#define PRIME_OS_PSP2
#define PRIME_OS_NAME "PSP2"
#define PRIME_CPU_ARM
#define PRIME_LITTLE_ENDIAN

#elif defined(__QNX__)

#define PRIME_OS_QNX
#define PRIME_OS_UNIX

// Assuming BlackBerry 10 for now
#define PRIME_OS_NAME "BB10"
#define PRIME_OS_BB10

#else

#error Unknown platform
#define PRIME_OS_NAME "Unknown platform"

#endif

#ifndef PRIME_OS_NAME
#error PRIME_OS_NAME not defined. Please fix Platform.h.
#endif

//
// Windows versions
//

#define PRIME_WINDOWS_95_WINVER 0x0400
#define PRIME_WINDOWS_95_WINDOWS 0x0400
#define PRIME_WINDOWS_95_WINNT 0x030a
#define PRIME_WINDOWS_95_IE 0

#define PRIME_WINDOWS_NT_WINVER 0x0400
#define PRIME_WINDOWS_NT_WINDOWS 0x0410
#define PRIME_WINDOWS_NT_WINNT 0x0400
#define PRIME_WINDOWS_NT_IE 0x0200

#define PRIME_WINDOWS_98_WINVER 0x0500
#define PRIME_WINDOWS_98_WINDOWS 0x0500
#define PRIME_WINDOWS_98_WINNT 0x0400
#define PRIME_WINDOWS_98_IE 0x0401

#define PRIME_WINDOWS_2000_WINVER 0x0500
#define PRIME_WINDOWS_2000_WINDOWS 0x0500
#define PRIME_WINDOWS_2000_WINNT 0x0500
#define PRIME_WINDOWS_2000_IE 0x0501

#define PRIME_WINDOWS_XP_WINVER 0x0501
#define PRIME_WINDOWS_XP_WINDOWS 0x0501
#define PRIME_WINDOWS_XP_WINNT 0x0501
#define PRIME_WINDOWS_XP_IE 0x0600

#define PRIME_WINDOWS_VISTA_WINVER 0x0600
#define PRIME_WINDOWS_VISTA_WINDOWS 0x0600
#define PRIME_WINDOWS_VISTA_WINNT 0x0600
#define PRIME_WINDOWS_VISTA_IE 0x0700

#define PRIME_WINDOWS_7_WINVER 0x0601
#define PRIME_WINDOWS_7_WINDOWS 0x0601
#define PRIME_WINDOWS_7_WINNT 0x0601
#define PRIME_WINDOWS_7_IE 0x0800

#define PRIME_WINDOWS_8_WINVER 0x0602
#define PRIME_WINDOWS_8_WINDOWS 0x0602
#define PRIME_WINDOWS_8_WINNT 0x0602
#define PRIME_WINDOWS_8_IE 0x0a00

#define PRIME_WINDOWS_10_WINVER 0x0a00
#define PRIME_WINDOWS_10_WINDOWS 0x0a00
#define PRIME_WINDOWS_10_WINNT 0x0a00
#define PRIME_WINDOWS_10_IE 0x0b00

// e.g., PRIME_WINVER_FOR(PRIME_WINDOWS_VISTA) => 0x0600
// Note that PRIME_WINDOWS_VISTA isn't defined anywhere (and mustn't be for these macros to work)

#define PRIME__WINVER_FOR(x) x##_WINVER
#define PRIME_WINVER_FOR(x) PRIME__WINVER_FOR(x)

#define PRIME__WIN32_WINDOWS_FOR(x) x##_WINVER
#define PRIME_WIN32_WINDOWS_FOR(x) PRIME__WIN32_WINDOWS_FOR(x)

#define PRIME__WIN32_WINNT_FOR(x) x##_WINVER
#define PRIME_WIN32_WINNT_FOR(x) PRIME__WIN32_WINNT_FOR(x)

#define PRIME__WIN32_IE_FOR(x) x##_WINVER
#define PRIME_WIN32_IE_FOR(x) PRIME__WIN32_IE_FOR(x)

//
// CPU detection
//

#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(__amd64__) || defined(_M_X64) || defined(PRIME_CPU_X64)

#define PRIME_CPU_NAME "x64"
#define PRIME_CPU_X64
#define PRIME_LITTLE_ENDIAN
#define PRIME_CPU_HAS_SSE

#elif defined(__i386__) || defined(__i386) || defined(_M_IX86) || defined(i386) || defined(PRIME_CPU_386)

#define PRIME_CPU_NAME "i386"
#define PRIME_CPU_386
#define PRIME_LITTLE_ENDIAN
#define PRIME_CPU_HAS_SSE

#elif defined(__POWERPC__) || defined(__POWERPC64__) || defined(__powerpc__) || defined(__powerpc64__) || defined(__ppc__) || defined(__ppc64__) || defined(__PPC__) || defined(__PPC64__) || defined(_M_PPC) || defined(_M_PPC_BE) || defined(_M_PPC_LE) || defined(PRIME_CPU_PPC) || defined(PRIME_CPU_PPC64)

#define PRIME_CPU_PPC

#if defined(__POWERPC64__) || defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || defined(PRIME_CPU_PPC64)

#define PRIME_CPU_NAME "ppc64"
#define PRIME_CPU_PPC64

#else

#define PRIME_CPU_NAME "ppc32"

#endif

#elif defined(__aarch64__) || defined(PRIME_CPU_AARCH64)

#define PRIME_CPU_NAME "aarch64"
#define PRIME_CPU_AARCH64
#define PRIME_CPU_ARM
#define PRIME_CPU_ARM64

#elif defined(__arm64__) || defined(__arm64) || defined(PRIME_CPU_ARM64)

#define PRIME_CPU_NAME "arm64"
#define PRIME_CPU_ARM64
#define PRIME_CPU_ARM

#elif defined(__arm__) || defined(__arm) || defined(PRIME_CPU_ARM)

// It doesn't seem to be possible to distinguish between armv5 and armv7
#define PRIME_CPU_NAME "arm"
#define PRIME_CPU_ARM

#elif defined(PRIME_CPU_X86_16)

#define PRIME_CPU_NAME "x86-16"
#define PRIME_CPU_X86_16
#define PRIME_LITTLE_ENDIAN

#else

#error Unknown CPU
#define PRIME_CPU_NAME "unknown CPU"

#endif

// Define PRIME_CPU_ANY_X86 for all X86 class CPUs
#if defined(PRIME_CPU_386) || defined(PRIME_CPU_X64) || defined(PRIME_CPU_X86_16)
#define PRIME_CPU_ANY_X86
#endif

//
// Byte order (the CPU detection might have figured this out).
//

#if !defined(PRIME_BIG_ENDIAN) && !defined(PRIME_LITTLE_ENDIAN)

#if defined(__BIG_ENDIAN__) || defined(_M_PPC_BE) || defined(_M_PPCBE)

#define PRIME_BIG_ENDIAN

#elif defined(__LITTLE_ENDIAN__) || defined(_M_PPC_LE) || defined(_M_PPCLE)

#define PRIME_LITTLE_ENDIAN

#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#define PRIME_LITTLE_ENDIAN

#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#define PRIME_BIG_ENDIAN

#else

#error Unable to determine byte order.

#endif

#elif defined(PRIME_LITTLE_ENDIAN) && defined(PRIME_BIG_ENDIAN)
#error _BOTH_ PRIME_BIG_ENDIAN and PRIME_LITTLE_ENDIAN are defined!
#endif

#if defined(PRIME_BIG_ENDIAN)
#define PRIME_ENDIAN_NAME "BE"
#define PRIME_IS_BIG_ENDIAN 1
#define PRIME_IS_LITTLE_ENDIAN 0
#elif defined(PRIME_LITTLE_ENDIAN)
#define PRIME_ENDIAN_NAME "LE"
#define PRIME_IS_BIG_ENDIAN 0
#define PRIME_IS_LITTLE_ENDIAN 1
#else
#error Neither PRIME_BIG_ENDIAN nor PRIME_LITTLE_ENDIAN are defined!
#endif

#define PRIME_PLATFORM_DESCRIPTION PRIME_OS_NAME " " PRIME_CPU_NAME " " PRIME_ENDIAN_NAME " " PRIME_COMPILER_NAME

//
// C99?
//

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define PRIME_C99
#endif

//
// C99 inttypes and stdint
//

// Assume inttypes.h is available unless we know otherwise - inttypes.h and/or stdint.h are turning up even on
// non-C99 platforms.
#define PRIME_COMPILER_HAS_INTTYPES

#if defined(_MSC_VER)

// Visual C++ 2010 has added stdint.h but not inttypes.h <passive-aggressive>since inttypes.h deals with printf,
// which nobody uses since iostreams were invented</passive-aggressive>.

#undef PRIME_COMPILER_HAS_INTTYPES

#if _MSC_VER >= 1600
#define PRIME_COMPILER_HAS_STDINT
#else
#define PRIME_COMPILER_HAS__INT64
#endif

#endif

#if defined(__TURBOC__)

#undef PRIME_COMPILER_HAS_INTTYPES

#if __TURBOC__ >= 0x0500
#define PRIME_COMPILER_HAS__INT64
#else
#define PRIME_NO_INT64
#endif

#endif

#if defined(PRIME_NO_C99_INTTYPES)

// Disable all C99 inttypes.

#elif defined(PRIME_COMPILER_HAS_INTTYPES)

#include <inttypes.h>
#include <stdint.h>

#ifndef PRId8
#error inttypes.h or stdint.h did not work. Try #including Platform.h earlier.
#endif

#else

// By including stdint.h we get the strict types, the macros to correctly define literal constants (e.g.,
// UINT8_C) and the MIN/MAX constants, but we don't get the C specific printf and scanf formatting macros.
#if defined(PRIME_COMPILER_HAS_STDINT)
#include <stdint.h>
#endif

//
// Figure out the strict types
//

#if UCHAR_MAX == 0xff

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef signed char int8_t;
typedef unsigned char uint8_t;

#define INT8_C(x) (x)
#define UINT8_C(x) (x##U)
#endif

// Assuming promotion to int.
#define PRId8 "d"
#define PRIi8 "i"
#define PRIo8 "o"
#define PRIu8 "u"
#define PRIx8 "x"
#define PRIX8 "X"

#else

#error unsigned char is not 8 bits.

#endif

#if USHRT_MAX == 0xffff

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef signed short int16_t;
typedef unsigned short uint16_t;

#define INT16_C(x) (x)
#define UINT16_C(x) (x##U)
#endif

// Assuming promotion to int.
#define PRId16 "d"
#define PRIi16 "i"
#define PRIo16 "o"
#define PRIu16 "u"
#define PRIx16 "x"
#define PRIX16 "X"

#else

#error unsigned short is not 16 bits.

#endif

#if UINT_MAX == 0xffffffffuL

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef signed int int32_t;
typedef unsigned int uint32_t;

#define INT32_C(x) (x)
#define UINT32_C(x) (x##U)
#endif

#define PRId32 "d"
#define PRIi32 "i"
#define PRIo32 "o"
#define PRIu32 "u"
#define PRIx32 "x"
#define PRIX32 "X"

#elif ULONG_MAX == 0xffffffffuL

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef signed long int32_t;
typedef unsigned long uint32_t;

#define INT32_C(x) (x##L)
#define UINT32_C(x) (x##UL)
#endif

#define PRId32 "ld"
#define PRIi32 "li"
#define PRIo32 "lo"
#define PRIu32 "lu"
#define PRIx32 "lx"
#define PRIX32 "lX"

#else

#error Neither unsigned int nor unsigned long are 32 bits.

#endif

#if !defined(PRIME_NO_INT64)

#if defined(PRIME_COMPILER_HAS__INT64)

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;

#define INT64_C(x) (x##I64)
#define UINT64_C(x) (x##UI64)
#endif

#define PRId64 "I64d"
#define PRIi64 "I64i"
#define PRIo64 "I64o"
#define PRIu64 "I64u"
#define PRIx64 "I64x"
#define PRIX64 "I64X"

#elif defined(ULLONG_MAX) && ULLONG_MAX == 0xffffffffffffffffuLL

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef signed long long int64_t;
typedef unsigned long long uint64_t;

#define INT64_C(x) (x##LL)
#define UINT64_C(x) (x##ULL)
#endif

#define PRId64 "lld"
#define PRIi64 "lli"
#define PRIo64 "llo"
#define PRIu64 "llu"
#define PRIx64 "llx"
#define PRIX64 "llX"

#elif ULONG_MAX > UINT32_C(0xffffffff) && ULONG_MAX == 0xffffffffffffffffuL

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef signed long int64_t;
typedef unsigned long uint64_t;

#define INT64_C(x) (x##L)
#define UINT64_C(x) (x##UL)
#endif

#define PRId64 "ld"
#define PRIi64 "li"
#define PRIo64 "lo"
#define PRIu64 "lu"
#define PRIx64 "lx"
#define PRIX64 "lX"

#else

#define PRIME_NO_INT64

#endif

#endif

#if SIZE_MAX == 0xffff || SIZE_MAX < UINT32_C(0xffffffff)

#define PRIME_SIZEOF_POINTER 2

#elif SIZE_MAX == 0xffffffff

#define PRIME_SIZEOF_POINTER 4

#elif SIZE_MAX > UINT32_C(0xffffffff) && SIZE_MAX == UINT64_C(0xffffffffffffffff)

#define PRIME_SIZEOF_POINTER 8

#else

#error size_t is not 16, 32 or 64 bits.

#endif

//
// Now imply everything else from the strict types
//

#if !defined(INT8_MAX)
#define INT8_MAX 127
#define INT8_MIN -128
#define UINT8_MAX 255
#endif

#if !defined(INT16_MAX)
#define INT16_MAX 32767
#define INT16_MIN -32768
#define UINT16_MAX 65535
#endif

#if !defined(INT32_MAX)
#define INT32_MAX INT32_C(2147483647)
#define INT32_MIN (-INT32_MAX - 1)
#define UINT32_MAX UINT32_C(4294967295)
#endif

#if !defined(INT64_MAX) && !defined(PRIME_NO_INT64)
#define INT64_MAX INT64_C(9223372036854775807)
#define INT64_MIN (-INT64_MAX - 1)
#define UINT64_MAX UINT64_C(18446744073709551615)
#endif

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef int8_t int_least8_t;
typedef uint8_t uint_least8_t;

#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST8_MAX INT8_MAX
#define UINT_LEAST8_MAX UINT8_MAX
#endif

#define PRIdLEAST8 PRId8
#define PRIiLEAST8 PRIi8
#define PRIoLEAST8 PRIo8
#define PRIuLEAST8 PRIu8
#define PRIxLEAST8 PRIx8
#define PRIXLEAST8 PRIX8

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef int16_t int_least16_t;
typedef uint16_t uint_least16_t;

#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST16_MAX INT16_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#endif

#define PRIdLEAST16 PRId16
#define PRIiLEAST16 PRIi16
#define PRIoLEAST16 PRIo16
#define PRIuLEAST16 PRIu16
#define PRIxLEAST16 PRIx16
#define PRIXLEAST16 PRIX16

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef int32_t int_least32_t;
typedef uint32_t uint_least32_t;

#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST32_MAX INT32_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#endif

#define PRIdLEAST32 PRId32
#define PRIiLEAST32 PRIi32
#define PRIoLEAST32 PRIo32
#define PRIuLEAST32 PRIu32
#define PRIxLEAST32 PRIx32
#define PRIXLEAST32 PRIX32

#ifdef INT64_MAX
#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef int64_t int_least64_t;
typedef uint64_t uint_least64_t;

#define INT_LEAST64_MIN INT64_MIN
#define INT_LEAST64_MAX INT64_MAX
#define UINT_LEAST64_MAX UINT64_MAX
#endif

#define PRIdLEAST64 PRId64
#define PRIiLEAST64 PRIi64
#define PRIoLEAST64 PRIo64
#define PRIuLEAST64 PRIu64
#define PRIxLEAST64 PRIx64
#define PRIXLEAST64 PRIX64
#endif

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef int8_t int_fast8_t;
typedef uint8_t uint_fast8_t;

#define INT_FAST8_MIN INT8_MIN
#define INT_FAST8_MAX INT8_MAX
#define UINT_FAST8_MAX UINT8_MAX
#endif

#define PRIdFAST8 PRId8
#define PRIiFAST8 PRIi8
#define PRIoFAST8 PRIo8
#define PRIuFAST8 PRIu8
#define PRIxFAST8 PRIx8
#define PRIXFAST8 PRIX8

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef int16_t int_fast16_t;
typedef uint16_t uint_fast16_t;

#define INT_FAST16_MIN INT16_MIN
#define INT_FAST16_MAX INT16_MAX
#define UINT_FAST16_MAX UINT16_MAX
#endif

#define PRIdFAST16 PRId16
#define PRIiFAST16 PRIi16
#define PRIoFAST16 PRIo16
#define PRIuFAST16 PRIu16
#define PRIxFAST16 PRIx16
#define PRIXFAST16 PRIX16

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef int32_t int_fast32_t;
typedef uint32_t uint_fast32_t;

#define INT_FAST32_MIN INT32_MIN
#define INT_FAST32_MAX INT32_MAX
#define UINT_FAST32_MAX UINT32_MAX
#endif

#define PRIdFAST32 PRId32
#define PRIiFAST32 PRIi32
#define PRIoFAST32 PRIo32
#define PRIuFAST32 PRIu32
#define PRIxFAST32 PRIx32
#define PRIXFAST32 PRIX32

#ifdef INT64_MAX
#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef int64_t int_fast64_t;
typedef uint64_t uint_fast64_t;

#define INT_FAST64_MIN INT64_MIN
#define INT_FAST64_MAX INT64_MAX
#define UINT_FAST64_MAX UINT64_MAX
#endif

#define PRIdFAST64 PRId64
#define PRIiFAST64 PRIi64
#define PRIoFAST64 PRIo64
#define PRIuFAST64 PRIu64
#define PRIxFAST64 PRIx64
#define PRIXFAST64 PRIX64
#endif

#ifdef INT64_MAX
#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

#ifndef INTMAX_C
#define INTMAX_C(x) INT64_C(x)
#endif

#ifndef UINTMAX_C
#define UINTMAX_C(x) UINT64_C(x)
#endif

#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX
#endif

#define PRIdMAX PRId64
#define PRIiMAX PRIi64
#define PRIoMAX PRIo64
#define PRIuMAX PRIu64
#define PRIxMAX PRIx64
#define PRIXMAX PRIX64
#else
#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef int32_t intmax_t;
typedef uint32_t uintmax_t;

#ifndef INTMAX_C
#define INTMAX_C(x) INT32_C(x)
#endif

#ifndef UINTMAX_C
#define UINTMAX_C(x) UINT32_C(x)
#endif

#define INTMAX_MIN INT32_MIN
#define INTMAX_MAX INT32_MAX
#define UINTMAX_MAX UINT32_MAX
#endif

#define PRIdMAX PRId32
#define PRIiMAX PRIi32
#define PRIoMAX PRIo32
#define PRIuMAX PRIu32
#define PRIxMAX PRIx32
#define PRIXMAX PRIX32
#endif

#if !defined(PRIME_COMPILER_HAS_STDINT)
typedef size_t uintptr_t;
typedef ptrdiff_t intptr_t;
#endif

#if PRIME_SIZEOF_POINTER == 4

#if !defined(PRIME_COMPILER_HAS_STDINT)
#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX UINT32_MAX
#define UINTPTR_MAX UINT32_MAX

#define INTPTR_C(x) INT32_C(x)
#define UINTPTR_C(x) UINT32_C(x)
#endif

#define PRIdPTR PRId32
#define PRIiPTR PRIi32
#define PRIoPTR PRIo32
#define PRIuPTR PRIu32
#define PRIxPTR PRIx32
#define PRIXPTR PRIX32

#elif PRIME_SIZEOF_POINTER == 8

#if !defined(PRIME_COMPILER_HAS_STDINT)
#define INTPTR_MIN INT64_MIN
#define INTPTR_MAX UINT64_MAX
#define UINTPTR_MAX UINT64_MAX

#define INTPTR_C(x) INT64_C(x)
#define UINTPTR_C(x) UINT64_C(x)
#endif

#define PRIdPTR PRId64
#define PRIiPTR PRIi64
#define PRIoPTR PRIo64
#define PRIuPTR PRIu64
#define PRIxPTR PRIx64
#define PRIXPTR PRIX64

#elif PRIME_SIZEOF_POINTER == 2

#if !defined(PRIME_COMPILER_HAS_STDINT)
#define INTPTR_MIN INT16_MIN
#define INTPTR_MAX UINT16_MAX
#define UINTPTR_MAX UINT16_MAX

#define INTPTR_C(x) INT16_C(x)
#define UINTPTR_C(x) UINT16_C(x)
#endif

#define PRIdPTR PRId16
#define PRIiPTR PRIi16
#define PRIoPTR PRIo16
#define PRIuPTR PRIu16
#define PRIxPTR PRIx16
#define PRIXPTR PRIX16

#else

#error Unsupported PRIME_SIZEOF_POINTER.

#endif

#endif

//
// Check our answers
//

#if !defined(PRIME_NO_C99_INTTYPES)

PRIME_COMPILE_TIME_ASSERT(sizeof(uint8_t) == 1);
PRIME_COMPILE_TIME_ASSERT(sizeof(int8_t) == 1);

PRIME_COMPILE_TIME_ASSERT(sizeof(uint16_t) == 2);
PRIME_COMPILE_TIME_ASSERT(sizeof(int16_t) == 2);

PRIME_COMPILE_TIME_ASSERT(sizeof(uint32_t) == 4);
PRIME_COMPILE_TIME_ASSERT(sizeof(int32_t) == 4);

#if !defined(PRIME_NO_INT64)
PRIME_COMPILE_TIME_ASSERT(sizeof(uint64_t) == 8);
PRIME_COMPILE_TIME_ASSERT(sizeof(int64_t) == 8);
#endif

#endif

//
// Work out sizes of non-strict types
//

#if UCHAR_MAX == 0xff
#define PRIME_SIZEOF_CHAR 1
#else
#error Cannot determine size of char.
#endif

#if USHRT_MAX == 0xffff
#define PRIME_SIZEOF_SHORT 2
#else
#error Cannot determine size of short.
#endif

#if UINT_MAX == 0xffffu
#define PRIME_SIZEOF_INT 2
#elif UINT_MAX == 0xffffffffu
#define PRIME_SIZEOF_INT 4
#elif UINT_MAX == 0xffffffffffffffffu
#define PRIME_SIZEOF_INT 8
#else
#error Cannot determine size of int.
#endif

#if ULONG_MAX == 0xffffuL
#define PRIME_SIZEOF_LONG 2
#elif ULONG_MAX == 0xffffffffuL
#define PRIME_SIZEOF_LONG 4
#elif ULONG_MAX == 0xffffffffffffffffuL
#define PRIME_SIZEOF_LONG 8
#else
#error Cannot determine size of long.
#endif

#if defined(ULLONG_MAX)
#if ULLONG_MAX == 0xffffffffffffffffuLL
#define PRIME_SIZEOF_LONG_LONG 8
#elif ULLONG_MAX == 0xffffffffuLL
#define PRIME_SIZEOF_LONG_LONG 4
#else
#error Cannot determine size of long long.
#endif
#else
#define PRIME_SIZEOF_LONG_LONG 0
#endif

#if !defined(PRIME_SIZEOF_POINTER)
#if defined(SIZE_MAX)
#if SIZE_MAX == 0xffffffffffffffffuLL
#define PRIME_SIZEOF_POINTER 8
#elif SIZE_MAX == 0xffffffffuLL
#define PRIME_SIZEOF_POINTER 4
#else
#error Unexpected size of pointer based on SIZE_MAX.
#endif
#else
#error Cannot determine size of pointer.
#endif
#endif

#define PRIME_PRIMITIVE_SIZES                                                                                                                                                                                    \
    "c" PRIME_STRINGIFY(PRIME_SIZEOF_CHAR) "/"                                                                                                                                                                   \
                                           "s" PRIME_STRINGIFY(PRIME_SIZEOF_SHORT) "/"                                                                                                                           \
                                                                                   "i" PRIME_STRINGIFY(PRIME_SIZEOF_INT) "/"                                                                                     \
                                                                                                                         "l" PRIME_STRINGIFY(PRIME_SIZEOF_LONG) "/"                                              \
                                                                                                                                                                "ll" PRIME_STRINGIFY(PRIME_SIZEOF_LONG_LONG) "/" \
                                                                                                                                                                                                             "p" PRIME_STRINGIFY(PRIME_SIZEOF_POINTER)

//
// Strict floating point types and the FloatMax type
//

// log10(2 ^ (significand_bits + 1)) + 1
#define PRIME_PRIg_FLOAT ".9g"
#define PRIME_PRIg_DOUBLE ".18g"
#define PRIME_PRIg_LONG_DOUBLE ".26Lg" // sufficient for 80 bit precision (x86)
#define PRIME_PRIg_LONG_DOUBLE_AS_DOUBLE ".18Lg" // sufficient for 80 bit precision (x86)

typedef float PrimeFloat32;

typedef double PrimeFloat64;

PRIME_COMPILE_TIME_ASSERT(sizeof(PrimeFloat32) == 4);
PRIME_COMPILE_TIME_ASSERT(sizeof(PrimeFloat64) == 8);

#if defined(LDBL_DIG)
#if !(LDBL_DIG > DBL_DIG)
#define PRIME_LONG_DOUBLE_IS_DOUBLE
#endif
#else
#define PRIME_NO_LONG_DOUBLE
#endif

#if defined(LDBL_DIG) && !defined(PRIME_LONG_DOUBLE_IS_DOUBLE)

#define PRIME_FLOATMAX_IS_LONG_DOUBLE

typedef long double PrimeFloatMax;

#if defined(__TURBOC__)
#define PRIME_STRTOFLOATMAX _strtold
#else
#define PRIME_STRTOFLOATMAX strtold
#endif

#define PRIME_PRIg_FLOATMAX PRIME_PRIg_LONG_DOUBLE
#define PRIME_PRIg_FLOATMAX_AS_DOUBLE PRIME_PRIg_LONG_DOUBLE_AS_DOUBLE

#else

#define PRIME_FLOATMAX_IS_DOUBLE

typedef double PrimeFloatMax;

#define PRIME_STRTOFLOATMAX strtod

#define PRIME_PRIg_FLOATMAX PRIME_PRIg_DOUBLE
#define PRIME_PRIg_FLOATMAX_AS_DOUBLE PRIME_PRIg_DOUBLE

#endif

#ifdef _MSC_VER

#define PRIME_ISINF(f) (!_finite((f))) // This will return true for NaN...
#define PRIME_ISNAN(f) _isnan((f))
#define PRIME_ISFINITE(f) _finite((f))

#elif defined(PRIME_CXX11_STL) && !defined(PRIME_OS_OSX)

#define PRIME_ISINF(f) std::isinf((f))
#define PRIME_ISNAN(f) std::isnan((f))
#define PRIME_ISFINITE(f) std::isfinite((f))

#elif defined(PRIME_CXX) && !defined(PRIME_OS_OSX)

#define PRIME_ISINF(f) ::isinf((f))
#define PRIME_ISNAN(f) ::isnan((f))
#define PRIME_ISFINITE(f) ::isfinite((f))

#else

// These are implemented as macros on Mac/iOS

#define PRIME_ISINF(f) isinf((f))
#define PRIME_ISNAN(f) isnan((f))
#define PRIME_ISFINITE(f) isfinite((f))

#endif

#define PRIME_VALIDFLOAT PRIME_ISFINITE

#if defined(__TURBOC__) && __TURBOC__ < 0x500
#define PRIME_BEGIN_NAMESPACE(name)
#define PRIME_END_NAMESPACE()
#define PRIME_NAMESPACE(name)
#else
#define PRIME_BEGIN_NAMESPACE(name) namespace name {
#define PRIME_END_NAMESPACE() }
#define PRIME_NAMESPACE(name) name
#endif

#ifdef PRIME_CXX

PRIME_BEGIN_NAMESPACE(Prime)

typedef PrimeFloat32 Float32;
typedef PrimeFloat64 Float64;
typedef PrimeFloatMax FloatMax;

PRIME_END_NAMESPACE()

#endif // PRIME_CXX

//
// Bool type for C
//

typedef int PrimeBool;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

//
// null and undefined
//

typedef struct {
    char unused;
} PrimeNullType;

typedef struct {
    char unused;
} PrimeUndefinedType;

#ifdef PRIME_CXX

PRIME_BEGIN_NAMESPACE(Prime)

typedef PrimeNullType Null;
static const Null null = Null();

typedef PrimeUndefinedType Undefined;
static const Undefined undefined = Undefined();

PRIME_END_NAMESPACE()

#endif

//
// PRIME_VA_COPY
// You must call va_end on the copied argptr
//

#if defined(va_copy)
#define PRIME_VA_COPY va_copy
#elif defined(__va_copy)
#define PRIME_VA_COPY __va_copy
#elif defined(_va_copy)
#define PRIME_VA_COPY _va_copy
#else
#define PRIME_VA_COPY(dst, src) ((dst) = (src))
#endif

//
// Hardware breakpoints
//
// (Named PRIME_ALWAYS_DEBUGGER to point out that it halts regardless of the build configuration.)
//

#if defined(__clang__) || defined(__GNUC__)
#define PRIME_ASM_GCC
#endif

#if defined(PRIME_ASM_GCC) && (defined(PRIME_CPU_386) || defined(PRIME_CPU_X64))

#define PRIME_ALWAYS_DEBUGGER() __asm__ volatile("int $3")

#elif PRIME_MSC_AND_NEWER(1310)

#if !defined(_XBOX_VER) && _MSC_VER < 1600
// Due to a bug in VC++ 2008 x64, you need to include math.h before intrin.h.
#include <intrin.h>
#include <math.h>
#endif

#define PRIME_ALWAYS_DEBUGGER() __debugbreak()

#elif defined(_MSC_VER)

#define PRIME_ALWAYS_DEBUGGER() \
    __asm { int 3 }             \
    ;

#elif defined(PRIME_OS_PS3_SPU)

#define PRIME_ALWAYS_DEBUGGER() __asm__ volatile("stop 0x3fff")

#elif defined(PRIME_OS_PS3_PPU)

#define PRIME_ALWAYS_DEBUGGER() __asm__ volatile("tw 31,%r1,%r1")

#elif defined(PRIME_OS_PSP)

#define PRIME_ALWAYS_DEBUGGER() \
    do {                        \
        __asm { \
                break 0               \
        }                       \
        ;                       \
    } while (0)

#elif defined(PRIME_OS_WII)

#define PRIME_ALWAYS_DEBUGGER()    \
    do {                           \
        register unsigned int was; \
        register unsigned int now; \
        asm volatile               \
        {                          \
            mfmsr was;             \
            ori now, was, 0x400;   \
            mtmsr now;             \
            mtmsr was;             \
        };                         \
    } while (0)

#elif defined(PRIME_OS_IOS)

#include <signal.h>

#define PRIME_ALWAYS_DEBUGGER() raise(SIGTRAP)

#elif defined(PRIME_OS_PSP2)

// Does this work on other SNC platforms?
#define PRIME_ALWAYS_DEBUGGER() __builtin_breakpoint(0)

#elif defined(__GNUC__)

// Should probably version check GCC!

#define PRIME_ALWAYS_DEBUGGER() __builtin_trap()

#elif PRIME_CLANG_HAS_BUILTIN(__builtin_trap)

#define PRIME_ALWAYS_DEBUGGER() __builtin_trap()

#elif defined(__TURBOC__)

#define PRIME_ALWAYS_DEBUGGER() __asm { int 3 }

#else

// Other UNIXes may support SIGTRAP?

#define PRIME_ALWAYS_DEBUGGER() ((void)0)

#endif

//
// PRIME_ALIGN, ALIGNOF
// No PRIME_ALIGNAS since Visual C++ doesn't support it (as of Visual C++ 2013)
// Use PRIME_ALIGNED_TYPE(n) in a union instead:
//     union {
//         PRIME_ALIGNED_TYPE(PRIME_ALIGNOF(SSEVec3)) align;
//         char raw[sizeof(SSEVec3) * 100];
//     };
//

#if defined(_MSC_VER) || defined(__WATCOMC__)

#define PRIME_ALIGN(a) __declspec(align(a))
#define PRIME_ALIGNOF(a) __alignof(a)

#if PRIME_MSC_AND_OLDER(1400)
#define PRIME_ALIGNED_TYPE(n) double
#endif

#elif defined(__GNUC__) || defined(__clang__) || defined(__SNC__)

#define PRIME_ALIGN(a) __attribute__((__aligned__(a)))
#define PRIME_ALIGNOF(a) __alignof(a)
#define PRIME_ALIGNED_TYPE(n) \
    PRIME_ALIGN(n)            \
    char

#endif

#ifndef PRIME_ALIGN
#define PRIME_ALIGN(a)
#endif

#ifndef PRIME_ALIGNOF
#define PRIME_ALIGNOF(a) 8
#endif

#ifndef PRIME_ALIGNED_TYPE
#ifdef PRIME_CXX
PRIME_BEGIN_NAMESPACE(Prime)
PRIME_BEGIN_NAMESPACE(Private)
#ifdef _MSC_VER
#pragma warning(disable : 4324)
#endif
template <size_t N>
struct AlignedType {
};
template <>
struct AlignedType<1> {
    char c[1];
};
template <>
struct PRIME_ALIGN(2) AlignedType<2> {
    char c[1];
};
template <>
struct PRIME_ALIGN(4) AlignedType<4> {
    char c[1];
};
template <>
struct PRIME_ALIGN(8) AlignedType<8> {
    char c[1];
};
template <>
struct PRIME_ALIGN(16) AlignedType<16> {
    char c[1];
};
#ifdef _MSC_VER
#pragma warning(default : 4324)
#endif
PRIME_END_NAMESPACE()
PRIME_END_NAMESPACE()

#define PRIME_ALIGNED_TYPE(n)         \
    PRIME_NAMESPACE(Prime::Private::) \
    AlignedType<n>
#endif
#endif

//
// Optimisation helpers
//

#if PRIME_GCC_AND_NEWER(2, 96, 0) || defined(__SNC__)
#define PRIME_LIKELY(x) __builtin_expect(!!(x), 1)
#define PRIME_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define PRIME_LIKELY(x) (x)
#define PRIME_UNLIKELY(x) (x)
#endif

//
// __VA_ARGS__ support
//

#if PRIME_MSC_AND_OLDER(1400)
#define PRIME_NO_VA_ARGS
#endif

//
// RTTI detection
//

#if defined(_MSC_VER)

#if !defined(_CPPRTTI)
#define PRIME_NO_RTTI
#endif

#elif defined(__SNC__)

#if !__option(rtti)
#define PRIME_NO_RTTI
#endif

#elif defined(__clang__)

#if defined(__has_feature)
#if !__has_feature(cxx_rtti)
#define PRIME_NO_RTTI
#endif
#elif __clang_major__ >= 3 && (!defined(__GXX_RTTI) || !__GXX_RTTI)
#define PRIME_NO_RTTI
#endif

#elif defined(__GNUC__)

#if PRIME_GCC_AND_NEWER(4, 3, 2) && (!defined(__GXX_RTTI) || !__GXX_RTTI)
#define PRIME_NO_RTTI
#endif

#endif

//
// Exceptions detection
//

#if defined(_MSC_VER)

#if !defined(_CPPUNWIND)
#define PRIME_NO_EXCEPTIONS
#endif

#elif defined(__SNC__)

#if !__option(exceptions)
#define PRIME_NO_EXCEPTIONS
#endif

#elif defined(__clang__)

#if defined(__has_feature)
#if !__has_feature(cxx_exceptions)
#define PRIME_NO_EXCEPTIONS
#endif
#elif __clang_major__ >= 3 && (!defined(__EXCEPTIONS) || !__EXCEPTIONS)
#define PRIME_NO_EXCEPTIONS
#endif

#elif defined(__GNUC__)

#if !defined(__EXCEPTIONS) || !__EXCEPTIONS
#define PRIME_NO_EXCEPTIONS
#endif

#endif

//
// Symbol Visibility/DLL Export/Import
// PRIME_EXPORT exports something from a library, PRIME_IMPORT specifies that something is imported from a library.
// PRIME_PRIVATE prevents something being exported from the library.
//

#if defined(PRIME_OS_WINDOWS)

#define PRIME_EXPORT __declspec(dllexport)
#define PRIME_IMPORT __declspec(dllimport)
#define PRIME_PRIVATE

#elif defined(__GNUC__) || defined(__clang__)

#define PRIME_EXPORT __attribute__((visibility("default")))
#define PRIME_IMPORT __attribute__((visibility("default")))
#define PRIME_PRIVATE __attribute__((visibility("hidden")))

#else

#define PRIME_EXPORT
#define PRIME_IMPORT
#define PRIME_PRIVATE

#endif

//
// PRIME_DEPRECATED
//

// TODO: for C++14, we can use [[deprecated]]

#if defined(__GNUC__) || defined(__clang__)
#define PRIME_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define PRIME_DEPRECATED __declspec(deprecated)
#else
#define PRIME_DEPRECATED
#endif

//
// PRIME_NORETURN
// Tag a function as never returning.
//

#if defined(__clang__)

#define PRIME_NORETURN __attribute__((noreturn))
#define PRIME_ANALYZER_NORETURN __attribute__((analyzer_noreturn))

#elif defined(__GNUC__) || defined(__SNC__)

#define PRIME_NORETURN __attribute__((noreturn))
#define PRIME_ANALYZER_NORETURN

#elif PRIME_MSC_AND_NEWER(1300)

#define PRIME_NORETURN __declspec(noreturn)
#define PRIME_ANALYZER_NORETURN

#else

#define PRIME_NORETURN
#define PRIME_ANALYZER_NORETURN

#endif

//
// inline for C code
//
// - Use PRIME_INLINE in header files
// - Use PRIME_STATIC_INLINE in source files
//

#if defined(PRIME_CXX)

#define PRIME_INLINE static inline
#define PRIME_STATIC_INLINE static inline

#elif defined(_MSC_VER)

#define PRIME_INLINE static __inline
#define PRIME_STATIC_INLINE static __inline

#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

#define PRIME_INLINE static inline
#define PRIME_STATIC_INLINE static inline

#elif defined(__GNUC__)

#define PRIME_INLINE static __inline
#define PRIME_STATIC_INLINE static __inline

#else

#define PRIME_INLINE static
#define PRIME_STATIC_INLINE static

#endif

//
// PRIME_FORCEINLINE
//

#if defined(_MSC_VER)

#define PRIME_FORCEINLINE __forceinline

#elif defined(__GNUC__) || defined(__clang__) || defined(__SNC__)

#define PRIME_FORCEINLINE inline __attribute__((always_inline))

#else

#define PRIME_FORCEINLINE inline

#endif

//
// Platform-sensible buffer sizes
//

#if defined(PRIME_OS_DOS)
#define PRIME_BIG_STACK_BUFFER_SIZE 512u
#define PRIME_HUGE_BUFFER_SIZE 16384u
#define PRIME_FILE_BUFFER_SIZE 16384u
#else
#define PRIME_BIG_STACK_BUFFER_SIZE 4096u
#define PRIME_HUGE_BUFFER_SIZE (1024u * 1024u * 1u)
#define PRIME_FILE_BUFFER_SIZE (32u * 1024u)
#endif

//
// extern "C" or not
//

#ifdef PRIME_CXX
#define PRIME_EXTERN_C extern "C"
#define PRIME_BEGIN_EXTERN_C extern "C" {
#define PRIME_END_EXTERN_C }
#else
#define PRIME_EXTERN_C
#define PRIME_BEGIN_EXTERN_C
#define PRIME_END_EXTERN_C
#endif

//
// Path separators
//

#ifdef PRIME_OS_WINDOWS

#define PRIME_PATH_SLASH '\\'
#define PRIME_PATH_SLASHES "\\/"
#define PRIME_PATH_SEPARATORS "\\:/"
#define PRIME_PATH_IS_SLASH(c) ((c) == '/' || (c) == '\\')
#define PRIME_PATH_IS_SEPARATOR(c) (PRIME_PATH_IS_SLASH(c) || (c) == ':')

#elif defined(PRIME_OS_UNIX)

#define PRIME_PATH_SLASH '/'
#define PRIME_PATH_SLASHES "/"
#define PRIME_PATH_SEPARATORS "/"
#define PRIME_PATH_IS_SLASH(c) ((c) == '/')
#define PRIME_PATH_IS_SEPARATOR(c) PRIME_PATH_IS_SLASH(c)

#else

#define PRIME_PATH_SLASH '/'
#define PRIME_PATH_SLASHES "/\\"
#define PRIME_PATH_SEPARATORS "/\\:"
#define PRIME_PATH_IS_SLASH(c) ((c) == '/' || (c) == '\\')
#define PRIME_PATH_IS_SEPARATOR(c) (PRIME_PATH_IS_SLASH(c) || (c) == ':')

#endif

//
// Standardise non-standard functions
//

// strtoimax, strtoumax
#if defined(PRIME_NO_INT64)
#define PRIME_STRTOIMAX strtol
#define PRIME_STRTOUMAX strtoul
#elif defined(_MSC_VER) || defined(__TURBOC__)
#if PRIME_MSC_AND_OLDER(1300)
#define PRIME_NO_STRTOIMAX
#else
#define PRIME_STRTOIMAX _strtoi64
#define PRIME_STRTOUMAX _strtoui64

// MSC clamps the return to INT64_MIN/INT64_MAX/UINT64_MAX, but doesn't
// set errno - so we can't detect errors. Need to keep checking for
// genuine strtoll support in future MSC, since the C++11 standard
// requires the use of errno.
#define PRIME_STRTOMAX_USES_ERRNO
#endif
#else
#define PRIME_STRTOIMAX strtoimax
#define PRIME_STRTOUMAX strtoumax

#define PRIME_STRTOMAX_USES_ERRNO
#define PRIME_STRTOFLOATMAX_USES_ERRNO
#endif

// strcasecmp, strncasecmp
#if defined(_MSC_VER) || defined(__BORLANDC__)
#define PRIME_STRCASECMP(a, b) _stricmp((a), (b))
#define PRIME_STRNCASECMP(a, b, n) _strnicmp((a), (b), (n))
#else
#define PRIME_STRCASECMP(a, b) strcasecmp((a), (b))
#define PRIME_STRNCASECMP(a, b, n) strncasecmp((a), (b), (n))
#endif

//
// Utility macros
//

#define PRIME_IMPLIED_COMPARISONS_OPERATORS(rhs_type)                   \
    bool operator!=(rhs_type rhs) const { return !operator==(rhs); }    \
    bool operator<=(rhs_type rhs) const { return !operator>(rhs); }     \
    bool operator>(rhs_type rhs) const { return rhs.operator<(*this); } \
    bool operator>=(rhs_type rhs) const { return !operator<(rhs); }

#define PRIME_PI 3.14159265358979323846264338327950288
#define PRIME_PI_DOUBLE ((double)PRIME_PI)
#define PRIME_PI_FLOAT ((float)PRIME_PI)

// PRIME_STRINGIFY(whatever) -> "whatever"
#define PRIME_STRINGIFY_PRIVATE(n) #n
#define PRIME_STRINGIFY(n) PRIME_STRINGIFY_PRIVATE(n)

#define PRIME_COUNTOF(arr) ((sizeof(arr) / sizeof((arr)[0])))

#define PRIME_MIN(a, b) ((a) < (b) ? (a) : (b))
#define PRIME_MAX(a, b) ((a) > (b) ? (a) : (b))
#define PRIME_ABS(a) ((a) < 0 ? -(a) : (a))
#define PRIME_CLAMP(n, minval, maxval) ((n) < (minval) ? (minval) : (n) > (maxval) ? (maxval) : (n))

#ifdef PRIME_CXX11
#define PRIME_UNCOPYABLE(className) \
private:                            \
    className(className&) = delete; \
    className& operator=(const className&) = delete;
#else
#define PRIME_UNCOPYABLE(className) \
private:                            \
    className(className&) { }       \
    className& operator=(const className&) { return *this; }
#endif

#define PRIME_UNCONSTRUCTABLE(className) \
private:                                 \
    className() { }                      \
    PRIME_UNCOPYABLE(className)

#define PRIME_UNUSED(expr) ((void)(expr))

// e.g. PRIME_BINARY(10000001) -> 129
#define PRIME_BINARY(x) ((unsigned char)PRIME_BINARY_(UINT32_C(0##x)))
#define PRIME_BINARY_(x) (x & 1 | x >> 2 & 2 | x >> 4 & 4 | x >> 6 & 8 | x >> 8 & 16 | x >> 10 & 32 | x >> 12 & 64 | x >> 14 & 128)

// C99 defines an offsetof but on at least one platform, it's screwed up.
#define PRIME_OFFSETOF(type, field) ((size_t) & ((type*)sizeof(type))->field - sizeof(type))

#define PRIME_SIZEOF_FIELD(type, field) sizeof(((type*)sizeof(type))->field)

// some_code will only run once (not thread safe though).
#define PRIME_ONCE(some_code)         \
    {                                 \
        static int primeDoneOnce = 0; \
        if (!primeDoneOnce) {         \
            primeDoneOnce = 1;        \
            {                         \
                some_code;            \
            }                         \
        }                             \
    }

// Super is private to prevent classes failing to declare their superclass
#define PRIME_SUPER(className) \
private:                       \
    typedef className Super;

//
// Byte order utilities
//

#define PRIME_BYTEPTR(ptr) ((uint8_t*)(ptr))

#ifndef PRIME_NO_INT64

#define PRIME_MAKE64(low, high) ((uint64_t)(low) | (((uint64_t)(high)) << 32))

#define PRIME_MAKE64_BYTES(a, b, c, d, e, f, g, h) ((uint64_t)((uint64_t)(a) | ((uint64_t)(b) << 8) | ((uint64_t)(c) << 16) | ((uint64_t)(d) << 24) | ((uint64_t)(e) << 32) | ((uint64_t)(f) << 40) | ((uint64_t)(g) << 48) | ((uint64_t)(h) << 56)))

#define PRIME_LOW32(n) ((uint32_t)(n))

#define PRIME_HIGH32(n) ((uint32_t)(((uint64_t)(n)) >> 32))

#define PRIME_BYTE64(n, b) ((uint8_t)(((uint64_t)(n)) >> ((b)*8)))

#ifdef PRIME_LITTLE_ENDIAN
#define PRIME_READ64LE(ptr) (*(const uint64_t*)PRIME_BYTEPTR(ptr))
#define PRIME_WRITE64LE(ptr, n) (*(uint64_t*)PRIME_BYTEPTR(ptr) = (uint64_t)(n))

#define PRIME_READ64BE(ptr) PRIME_MAKE64_BYTES(PRIME_BYTEPTR(ptr)[7], \
    PRIME_BYTEPTR(ptr)[6],                                            \
    PRIME_BYTEPTR(ptr)[5],                                            \
    PRIME_BYTEPTR(ptr)[4],                                            \
    PRIME_BYTEPTR(ptr)[3],                                            \
    PRIME_BYTEPTR(ptr)[2],                                            \
    PRIME_BYTEPTR(ptr)[1],                                            \
    PRIME_BYTEPTR(ptr)[0])

#define PRIME_WRITE64BE(ptr, n) (PRIME_BYTEPTR(ptr)[7] = PRIME_BYTE64(n, 0), \
    PRIME_BYTEPTR(ptr)[6] = PRIME_BYTE64(n, 1),                              \
    PRIME_BYTEPTR(ptr)[5] = PRIME_BYTE64(n, 2),                              \
    PRIME_BYTEPTR(ptr)[4] = PRIME_BYTE64(n, 3),                              \
    PRIME_BYTEPTR(ptr)[3] = PRIME_BYTE64(n, 4),                              \
    PRIME_BYTEPTR(ptr)[2] = PRIME_BYTE64(n, 5),                              \
    PRIME_BYTEPTR(ptr)[1] = PRIME_BYTE64(n, 6),                              \
    PRIME_BYTEPTR(ptr)[0] = PRIME_BYTE64(n, 7))

#define PRIME_WRITE64 PRIME_WRITE64LE
#define PRIME_READ64 PRIME_READ64LE
#else
#define PRIME_READ64LE(ptr) PRIME_MAKE64_BYTES(PRIME_BYTEPTR(ptr)[0], \
    PRIME_BYTEPTR(ptr)[1],                                            \
    PRIME_BYTEPTR(ptr)[2],                                            \
    PRIME_BYTEPTR(ptr)[3],                                            \
    PRIME_BYTEPTR(ptr)[4],                                            \
    PRIME_BYTEPTR(ptr)[5],                                            \
    PRIME_BYTEPTR(ptr)[6],                                            \
    PRIME_BYTEPTR(ptr)[7])

#define PRIME_WRITE64LE(ptr, n) (PRIME_BYTEPTR(ptr)[0] = PRIME_BYTE64(n, 0), \
    PRIME_BYTEPTR(ptr)[1] = PRIME_BYTE64(n, 1),                              \
    PRIME_BYTEPTR(ptr)[2] = PRIME_BYTE64(n, 2),                              \
    PRIME_BYTEPTR(ptr)[3] = PRIME_BYTE64(n, 3),                              \
    PRIME_BYTEPTR(ptr)[4] = PRIME_BYTE64(n, 4),                              \
    PRIME_BYTEPTR(ptr)[5] = PRIME_BYTE64(n, 5),                              \
    PRIME_BYTEPTR(ptr)[6] = PRIME_BYTE64(n, 6),                              \
    PRIME_BYTEPTR(ptr)[7] = PRIME_BYTE64(n, 7))

#define PRIME_READ64BE(ptr) (*(const uint64_t*)PRIME_BYTEPTR(ptr))
#define PRIME_WRITE64BE(ptr, n) (*(uint64_t*)PRIME_BYTEPTR(ptr) = (uint64_t)(n))

#define PRIME_WRITE64 PRIME_WRITE64BE
#define PRIME_READ64 PRIME_READ64BE
#endif

#if PRIME_GCC_AND_NEWER(4, 3, 0) || PRIME_CLANG_HAS_BUILTIN(__builtin_bswap64)
#define PRIME_SWAP64(n) __builtin_bswap64((n))
#else
#define PRIME_SWAP64(n) ( \
    (uint64_t)(((((uint64_t)(n)) & UINT64_C(0xff00000000000000)) >> 56) | ((((uint64_t)(n)) & UINT64_C(0x00000000000000ff)) << 56) | ((((uint64_t)(n)) & UINT64_C(0x00ff000000000000)) >> 40) | ((((uint64_t)(n)) & UINT64_C(0x000000000000ff00)) << 40) | ((((uint64_t)(n)) & UINT64_C(0x0000ff0000000000)) >> 24) | ((((uint64_t)(n)) & UINT64_C(0x0000000000ff0000)) << 24) | ((((uint64_t)(n)) & UINT64_C(0x000000ff00000000)) >> 8) | ((((uint64_t)(n)) & UINT64_C(0x00000000ff000000)) << 8)))
#endif

#define PRIME_SWAP64_IN_PLACE(ptr) PRIME_WRITE64LE((ptr), PRIME_READ64BE((ptr)))

#ifdef PRIME_LITTLE_ENDIAN
#define PRIME_SWAP64LE(n) (n)
#define PRIME_SWAP64BE(n) PRIME_SWAP64((n))
#else
#define PRIME_SWAP64LE(n) PRIME_SWAP64((n))
#define PRIME_SWAP64BE(n) (n)
#endif

#endif

#define PRIME_MAKE32(low, high) \
    ((uint32_t)(((uint32_t)(low)) | (((uint32_t)(high)) << 16)))

#define PRIME_MAKE32_BYTES(lowest, low, high, highest) \
    ((uint32_t)((uint32_t)(lowest) | ((uint32_t)(low) << 8) | ((uint32_t)(high) << 16) | ((uint32_t)(highest) << 24)))

#define PRIME_FOURCC_LE(a, b, c, d) PRIME_MAKE32_BYTES(a, b, c, d)

#define PRIME_FOURCC_BE(a, b, c, d) PRIME_MAKE32_BYTES(d, c, b, a)

#define PRIME_BYTE32(n, b) ((uint8_t)(((uint32_t)(n)) >> (b * 8)))

#define PRIME_LOW16(n) ((uint16_t)(n))
#define PRIME_HIGH16(n) ((uint16_t)(((uint32_t)(n)) >> 16))

#ifdef PRIME_LITTLE_ENDIAN
#define PRIME_READ32LE(ptr) (*(const uint32_t*)PRIME_BYTEPTR(ptr))
#define PRIME_WRITE32LE(ptr, n) (*(uint32_t*)PRIME_BYTEPTR(ptr) = (uint32_t)(n))

#define PRIME_READ32BE(ptr) PRIME_MAKE32_BYTES(PRIME_BYTEPTR(ptr)[3], \
    PRIME_BYTEPTR(ptr)[2],                                            \
    PRIME_BYTEPTR(ptr)[1],                                            \
    PRIME_BYTEPTR(ptr)[0])

#define PRIME_WRITE32BE(ptr, n) (PRIME_BYTEPTR(ptr)[3] = PRIME_BYTE32(n, 0), \
    PRIME_BYTEPTR(ptr)[2] = PRIME_BYTE32(n, 1),                              \
    PRIME_BYTEPTR(ptr)[1] = PRIME_BYTE32(n, 2),                              \
    PRIME_BYTEPTR(ptr)[0] = PRIME_BYTE32(n, 3))

#define PRIME_WRITE32 PRIME_WRITE32LE
#define PRIME_READ32 PRIME_READ32LE
#else
#define PRIME_READ32LE(ptr) PRIME_MAKE32_BYTES(PRIME_BYTEPTR(ptr)[0], \
    PRIME_BYTEPTR(ptr)[1],                                            \
    PRIME_BYTEPTR(ptr)[2],                                            \
    PRIME_BYTEPTR(ptr)[3])

#define PRIME_WRITE32LE(ptr, n) (PRIME_BYTEPTR(ptr)[0] = PRIME_BYTE32(n, 0), \
    PRIME_BYTEPTR(ptr)[1] = PRIME_BYTE32(n, 1),                              \
    PRIME_BYTEPTR(ptr)[2] = PRIME_BYTE32(n, 2),                              \
    PRIME_BYTEPTR(ptr)[3] = PRIME_BYTE32(n, 3))

#define PRIME_READ32BE(ptr) (*(const uint32_t*)PRIME_BYTEPTR(ptr))
#define PRIME_WRITE32BE(ptr, n) (*(uint32_t*)PRIME_BYTEPTR(ptr) = (uint32_t)(n))

#define PRIME_WRITE32 PRIME_WRITE32BE
#define PRIME_READ32 PRIME_READ32BE
#endif

#if PRIME_GCC_AND_NEWER(4, 3, 0) || PRIME_CLANG_HAS_BUILTIN(__builtin_bswap32)
#define PRIME_SWAP32(n) __builtin_bswap32((n))
#else
#define PRIME_SWAP32(n) ((uint32_t)((((uint32_t)(n)) >> 24) | (((uint32_t)(n)) << 24) | ((((uint32_t)(n)) & UINT32_C(0x00ff0000)) >> 8) | ((((uint32_t)(n)) & UINT32_C(0x0000ff00)) << 8)))
#endif

#define PRIME_SWAP32_IN_PLACE(ptr) PRIME_WRITE32LE((ptr), PRIME_READ32BE((ptr)))

#ifdef PRIME_LITTLE_ENDIAN
#define PRIME_SWAP32LE(n) (n)
#define PRIME_SWAP32BE(n) PRIME_SWAP32((n))
#else
#define PRIME_SWAP32LE(n) PRIME_SWAP32((n))
#define PRIME_SWAP32BE(n) (n)
#endif

#define PRIME_MAKE16_BYTES(low, high) ((uint16_t)(((uint16_t)(low)) | (((uint16_t)(high)) << 8)))

#define PRIME_LOW8(n) ((uint8_t)(n))

#define PRIME_HIGH8(n) ((uint8_t)(((unsigned int)(n)) >> 8))

#ifdef PRIME_LITTLE_ENDIAN
#define PRIME_READ16LE(ptr) (*(const uint16_t*)PRIME_BYTEPTR(ptr))
#define PRIME_WRITE16LE(ptr, n) (*(uint16_t*)PRIME_BYTEPTR(ptr) = (uint16_t)(n))

#define PRIME_READ16BE(ptr) PRIME_MAKE16_BYTES(PRIME_BYTEPTR(ptr)[1], PRIME_BYTEPTR(ptr)[0])

#define PRIME_WRITE16BE(ptr, n) (PRIME_BYTEPTR(ptr)[1] = PRIME_LOW8(n), \
    PRIME_BYTEPTR(ptr)[0] = PRIME_HIGH8(n))

#define PRIME_WRITE16 PRIME_WRITE16LE
#define PRIME_READ16 PRIME_READ16LE
#else
#define PRIME_READ16LE(ptr) PRIME_MAKE16_BYTES(PRIME_BYTEPTR(ptr)[0], PRIME_BYTEPTR(ptr)[1])

#define PRIME_WRITE16LE(ptr, n) (PRIME_BYTEPTR(ptr)[0] = PRIME_LOW8(n), \
    PRIME_BYTEPTR(ptr)[1] = PRIME_HIGH8(n))

#define PRIME_READ16BE(ptr) (*(const uint16_t*)PRIME_BYTEPTR(ptr))
#define PRIME_WRITE16BE(ptr, n) (*(uint16_t*)PRIME_BYTEPTR(ptr) = (uint16_t)(n))

#define PRIME_WRITE16 PRIME_WRITE16BE
#define PRIME_READ16 PRIME_READ16BE
#endif

#define PRIME_SWAP16(n) ((uint16_t)((((uint16_t)(n)) >> 8) | (((uint16_t)(n)) << 8)))
#define PRIME_SWAP16_IN_PLACE(ptr) PRIME_WRITE16LE((ptr), PRIME_READ16BE((ptr)))

#ifdef PRIME_LITTLE_ENDIAN
#define PRIME_SWAP16LE(n) (n)
#define PRIME_SWAP16BE(n) PRIME_SWAP16((n))
#else
#define PRIME_SWAP16LE(n) PRIME_SWAP16((n))
#define PRIME_SWAP16BE(n) (n)
#endif

union PrimeFloatInt32 {
    PrimeFloat32 f;
    uint32_t u;
    int32_t s;
};

#define PRIME_FLOAT32_TO_U32(ptr) (((PrimeFloatInt32*)(ptr))->u)
#define PRIME_U32_TO_FLOAT32(ptr) (((PrimeFloatInt32*)(ptr))->f)
// There's no PRIME_SWAP_FLOAT32 because it's not portable.
#define PRIME_SWAP_FLOAT32_IN_PLACE(ptr) PRIME_SWAP32_IN_PLACE(&((PrimeFloatInt32*)(ptr))->u)

#if !defined(PRIME_NO_INT64) && !defined(PRIME_NO_FLOAT64)

union PrimeFloatInt64 {
    PrimeFloat64 f;
    uint64_t u;
    int64_t s;
};

#define PRIME_FLOAT64_TO_U64(ptr) (((PrimeFloatInt64*)(ptr))->u)
#define PRIME_U64_TO_FLOAT64(ptr) (((PrimeFloatInt64*)(ptr))->f)
// There's no PRIME_SWAP_FLOAT64 because it's not a good idea.
#define PRIME_SWAP_FLOAT64_IN_PLACE(ptr) PRIME_SWAP64_IN_PLACE(&((PrimeFloatInt64*)(ptr))->u)

#endif

#endif
