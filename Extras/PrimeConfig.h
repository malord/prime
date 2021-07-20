// Copyright 2000-2021 Mark H. P. Lord
// This file needs to be placed in the folder _above_ Prime, then modified depending on which supporting libraries
// you wish to use alongside Prime.

#ifndef PRIMECONFIG_H
#define PRIMECONFIG_H

// Minimum version of Windows to support. Default is 95 for Visual C++ 6, otherwise XP.
// #define PRIME_WINVER PRIME_WINDOWS_XP

// Override assertions
// #include <assert.h>
// #define PRIME_CUSTOM_ASSERT assert

// Un-comment this to disable exceptions
// #define PRIME_NO_EXCEPTIONS

// Un-comment to disable threads (builds without thread safety)
// #define PRIME_NO_THREADS

// Un-comment to disable Grand Central Dispatch 
// #define PRIME_NO_GCD

// Un-comment this to declare iostream operators where supported. In most cases you can define this in your
// own project since the iostream operators are header-only.
// #define PRIME_ENABLE_IOSTREAMS

// Use UTF-8 on Windows 95, 98 and ME. This would be necessary in an internationalised application, but for
// English applications where filenames are passed through unmodified, there's a performance penalty for no benefit.
// #define PRIME_WIN98_UTF8

//
// Libraries
//

// Un-comment this if you don't want to link with zlib
// #define PRIME_NO_ZLIB

// Un-comment this if you don't want to link with iconv on available platforms
// #define PRIME_NO_ICONV

// Un-comment this if you don't want to link with miniconv when iconv is disabled
// #define PRIME_NO_MINICONV

// Un-comment this if you don't want to link with Oniguruma
// #define PRIME_NO_ONIGURUMA

// Un-comment to disable MySQL support
// #define PRIME_NO_MYSQL

// Un-comment to disable SQLite support
// #define PRIME_NO_SQLITE

// Un-comment to disable OpenSSL support
// #define PRIME_NO_OPENSSL

// Un-comment this if you don't want to link with utf8rewind
// #define PRIME_NO_UTF8REWIND

#ifdef PRIME_OS_IOS
	#define PRIME_NO_OPENSSL
#endif

#endif
