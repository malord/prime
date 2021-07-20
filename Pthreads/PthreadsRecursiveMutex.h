// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PTHREADS_PTHREADSRECURSIVEMUTEX_H
#define PRIME_PTHREADS_PTHREADSRECURSIVEMUTEX_H

#include "PthreadsMutex.h"

namespace Prime {

/// A mutex which allows balanced locks/unlocks from the same thread.
class PthreadsRecursiveMutex : public PthreadsMutex {
public:
    PthreadsRecursiveMutex() { }

    explicit PthreadsRecursiveMutex(Log* log, const char* debugName = NULL)
        : PthreadsMutex(log, debugName, PthreadsMutex::Attributes().setRecursive(true))
    {
    }

    bool init(Log* log, const char* debugName = NULL)
    {
        return PthreadsMutex::init(log, debugName, PthreadsMutex::Attributes().setRecursive(true));
    }

    PRIME_UNCOPYABLE(PthreadsRecursiveMutex);
};
}

#endif
