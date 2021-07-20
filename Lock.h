// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_LOCK_H
#define PRIME_LOCK_H

#include "RefCounting.h"
#include "ScopedLock.h"

namespace Prime {

/// A polymorphic lock.
class PRIME_PUBLIC Lock : public RefCounted {
public:
    typedef Prime::ScopedLock<Lock> ScopedLock;

    Lock() PRIME_NOEXCEPT { }

    virtual ~Lock() { }

    virtual void lock() = 0;

    virtual void unlock() = 0;

    PRIME_UNCOPYABLE(Lock);
};
}

#endif
