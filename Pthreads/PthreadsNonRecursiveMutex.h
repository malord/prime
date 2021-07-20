// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PTHREADS_PTHREADSNONRECURSIVEMUTEX_H
#define PRIME_PTHREADS_PTHREADSNONRECURSIVEMUTEX_H

#include "PthreadsMutex.h"

namespace Prime {

/// Note that a PthreadsNonRecursiveMutex cannot be unlocked by any thread except that which locked it.
class PthreadsNonRecursiveMutex : public PthreadsMutex {
public:
    PthreadsNonRecursiveMutex() { }

    explicit PthreadsNonRecursiveMutex(Log* log, const char* debugName = NULL)
        : PthreadsMutex(log, debugName, PthreadsMutex::Attributes().setRecursive(false))
    {
    }

    bool init(Log* log, const char* debugName = NULL)
    {
        return PthreadsMutex::init(log, debugName, PthreadsMutex::Attributes().setRecursive(false));
    }

    PRIME_UNCOPYABLE(PthreadsNonRecursiveMutex);
};
}

#endif
