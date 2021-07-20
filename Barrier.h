// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_BARRIER_H
#define PRIME_BARRIER_H

#include "Config.h"

#if 1

#include "Emulated/EmulatedBarrier.h"

namespace Prime {

/// Use this rather than using EmulatedBarrier directly to allow optimised implementations in the future.
typedef EmulatedBarrier Barrier;
}

#endif

#endif
