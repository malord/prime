// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DYNAMICLIBRARY_H
#define PRIME_DYNAMICLIBRARY_H

#include "Config.h"

#define PRIME_HAVE_DYNAMICLIBRARY

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsDynamicLibrary.h"

namespace Prime {
typedef WindowsDynamicLibrary DynamicLibrary;
}

#elif defined(PRIME_OS_UNIX)

#include "Unix/UnixDynamicLibrary.h"

namespace Prime {
typedef UnixDynamicLibrary DynamicLibrary;
}

#else

#undef PRIME_HAVE_DYNAMICLIBRARY

#endif

#endif
