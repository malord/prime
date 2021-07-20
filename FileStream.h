// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_FILESTREAM_H
#define PRIME_FILESTREAM_H

#include "Config.h"

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsFileStream.h"

namespace Prime {
/// Typedef to the platform FileStream type.
typedef WindowsFileStream FileStream;
}

#elif defined(PRIME_OS_UNIX)

#include "Unix/UnixFileStream.h"

namespace Prime {
/// Typedef to the platform FileStream type.
typedef UnixFileStream FileStream;
}

#else

#include "StdioStream.h"

namespace Prime {
/// Typedef to the platform FileStream type.
typedef StdioStream FileStream;
}

#endif

#endif
