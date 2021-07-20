// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DIRECTORYREADER_H
#define PRIME_DIRECTORYREADER_H

#include "Config.h"

#define PRIME_HAVE_DIRECTORYREADER

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsDirectoryReader.h"

namespace Prime {
typedef WindowsDirectoryReader DirectoryReader;
}

#elif defined(PRIME_OS_UNIX)

#include "Unix/UnixDirectoryReader.h"

namespace Prime {
typedef UnixDirectoryReader DirectoryReader;
}

#else

#undef PRIME_HAVE_DIRECTORYREADER

#endif

#endif
