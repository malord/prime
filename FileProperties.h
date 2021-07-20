// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_FILEPROPERTIES_H
#define PRIME_FILEPROPERTIES_H

#include "Config.h"

#define PRIME_HAVE_FILEPROPERTIES

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsFileProperties.h"

namespace Prime {
typedef WindowsFileProperties FileProperties;
}

#define PRIME_FILEPROPERTIES_IS_DIRECTORYREADER

#elif defined(PRIME_OS_UNIX)

#include "Unix/UnixFileProperties.h"

#define PRIME_HAVE_FILEPROPERTIES_STATHANDLE

namespace Prime {
typedef UnixFileProperties FileProperties;
}

#else

#undef PRIME_HAVE_FILEPROPERTIES

#endif

#endif
