// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WILDCARDEXPANSION_H
#define PRIME_WILDCARDEXPANSION_H

#include "Config.h"

#define PRIME_HAVE_WILDCARDEXPANSION

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsWildcardExpansion.h"

namespace Prime {
typedef WindowsWildcardExpansion WildcardExpansion;
}

#elif defined(PRIME_OS_UNIX) && !defined(PRIME_OS_ANDROID)

#include "Unix/UnixWildcardExpansion.h"

namespace Prime {
typedef UnixWildcardExpansion WildcardExpansion;
}

#else

#include "Emulated/EmulatedWildcardExpansion.h"

namespace Prime {
#ifdef PRIME_HAVE_DIRECTORYREADERWILDCARDEXPANSION
typedef EmulatedWildcardExpansion WildcardExpansion;
#else
#undef PRIME_HAVE_WILDCARDEXPANSION
#endif
}

#endif

#endif
