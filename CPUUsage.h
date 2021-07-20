// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CPUUSAGE_H
#define PRIME_CPUUSAGE_H

#include "Config.h"

#if defined(PRIME_OS_OSX) && !defined(PRIME_OS_IOS)

#include "OSX/OSXCPUUsage.h"

namespace Prime {
/// Typedef to the platform CPUUsage.
typedef OSXCPUUsage CPUUsage;
}

#else

#define PRIME_NO_CPUUSAGE

#endif

#endif
