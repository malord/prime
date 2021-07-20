// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CONDITION_H
#define PRIME_CONDITION_H

#include "Config.h"

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsCondition.h"

namespace Prime {
/// Condition variable.
typedef WindowsCondition Condition;
}

#elif defined(PRIME_OS_UNIX)

#include "Pthreads/PthreadsCondition.h"

namespace Prime {
/// Condition variable.
typedef PthreadsCondition Condition;
}

#endif

#endif
