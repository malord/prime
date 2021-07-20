// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNOWNEDPTR_H
#define PRIME_UNOWNEDPTR_H

#include "Config.h"

namespace Prime {

/// Make it clear that a pointer is a non-owning reference.
template <typename T>
class UnownedPtr {
public:
    UnownedPtr()
        : _p(NULL)
    {
    }

    UnownedPtr(T* p)
        : _p(p)
    {
    }

    void reset(T* p = NULL)
    {
        _p = p;
    }

    UnownedPtr& operator=(T* p)
    {
        _p = p;
        return *this;
    }

    void swap(UnownedPtr& other)
    {
        std::swap(_p, other._p);
    }

    T* get() const { return _p; }

    T* operator->() const { return _p; }

    T& operator*() const { return *_p; }

    operator T*() const { return _p; }

private:
    T* _p;
};
}

#endif
