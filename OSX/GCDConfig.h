// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OSX_GCDCONFIG_H
#define PRIME_OSX_GCDCONFIG_H

#include "../Config.h"

#if defined(PRIME_OS_OSX) && !defined(PRIME_NO_GCD)

#include <dispatch/dispatch.h>

#else

#define PRIME_NO_GCD

#endif

#endif
