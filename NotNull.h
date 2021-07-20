// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_NOTNULL_H
#define PRIME_NOTNULL_H

#include "Config.h"

namespace Prime {

/// Wrap a function parameter in this to enforce a not-null contract. e.g., NotNull<const Object*> object.
template <typename Pointer>
class NotNull {
public:
    NotNull(Pointer pointer) PRIME_NOEXCEPT : _pointer(pointer)
    {
        PRIME_DEBUG_ASSERT(pointer != NULL);
    }

    NotNull(const NotNull& copy) PRIME_NOEXCEPT : _pointer(copy._pointer)
    {
    }

    NotNull& operator=(const NotNull& copy) PRIME_NOEXCEPT
    {
        _pointer = copy._pointer;
        return *this;
    }

    NotNull& operator=(Pointer assign) PRIME_NOEXCEPT
    {
        _pointer = assign;
        PRIME_DEBUG_ASSERT(_pointer != NULL);
        return *this;
    }

#ifdef PRIME_COMPILER_RVALUEREF
    NotNull(const NotNull&& move) PRIME_NOEXCEPT : _pointer(move._pointer)
    {
    }

    NotNull& operator=(const NotNull&& move) PRIME_NOEXCEPT
    {
        _pointer = move._pointer;
        return *this;
    }
#endif

    Pointer get() const PRIME_NOEXCEPT
    {
        return _pointer;
    }

    operator Pointer() const PRIME_NOEXCEPT { return _pointer; }
    Pointer operator->() const PRIME_NOEXCEPT { return _pointer; }

    bool operator==(const Pointer& rhs) const PRIME_NOEXCEPT { return _pointer == rhs; }
    bool operator!=(const Pointer& rhs) const PRIME_NOEXCEPT { return _pointer != rhs; }
    bool operator<(const Pointer& rhs) const PRIME_NOEXCEPT { return _pointer < rhs; }
    bool operator<=(const Pointer& rhs) const PRIME_NOEXCEPT { return _pointer <= rhs; }
    bool operator>(const Pointer& rhs) const PRIME_NOEXCEPT { return _pointer > rhs; }
    bool operator>=(const Pointer& rhs) const PRIME_NOEXCEPT { return _pointer >= rhs; }

private:
#ifdef PRIME_COMPILER_NULLPTR
    NotNull(std::nullptr_t) PRIME_NOEXCEPT : _pointer(NULL)
    {
    }

    NotNull& operator=(std::nullptr_t) PRIME_NOEXCEPT
    {
        return *this;
    }
#endif

    NotNull(int) PRIME_NOEXCEPT : _pointer(NULL)
    {
    }

    NotNull& operator=(int) PRIME_NOEXCEPT
    {
        return *this;
    }

    Pointer _pointer;
};
}

#endif
