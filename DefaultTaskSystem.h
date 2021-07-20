// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DEFAULTTASKSYSTEM_H
#define PRIME_DEFAULTTASKSYSTEM_H

#include "Config.h"

#if defined(PRIME_OS_OSX) && !defined(PRIME_NO_GCD)

#include "OSX/GCDTaskSystem.h"

namespace Prime {
/// Typedef'd to the appropriate TaskSystem for the target platform.
typedef GCDTaskSystem DefaultTaskSystem;
}

#else

#include "ThreadPoolTaskSystem.h"

namespace Prime {
/// Typedef'd to the appropriate TaskSystem for the target platform.
typedef ThreadPoolTaskSystem DefaultTaskSystem;
}

#endif

#endif
