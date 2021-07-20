// Copyright 2000-2021 Mark H. P. Lord

// ScopedPtr and ScopedArrayPtr are std::auto_ptr (and an array deleting equivalent) without the ownership passing
// madness - on pre-C++11 compilers, ownership must be explicitly passed, e.g.: auto_ptr1.move(auto_ptr2), from
// C++11 onwards, pointers can be moved.

#ifndef PRIME_SCOPEDPTR_H
#define PRIME_SCOPEDPTR_H

#include "Config.h"

namespace Prime {

//
// ScopedPtrDeleter
//

/// Default Deleter for ScopedPtr
template <typename Type>
class ScopedPtrDeleter {
public:
    static void deletePointer(Type* pointer) { delete pointer; }
};

//
// ScopedPtr
//

/// Differs from std::auto_ptr in that you must explicitly pass ownership of the pointer between ScopedPtr
/// instances, e.g., auto_ptr1.move(auto_ptr2);
template <typename Type, typename Deleter = ScopedPtrDeleter<Type>>
class ScopedPtr {
public:
    /// Optionally assign the object to be deleted.
    explicit ScopedPtr(Type* assign = NULL)
        : _pointer(assign)
    {
    }

#ifdef PRIME_COMPILER_RVALUEREF

    ScopedPtr(ScopedPtr&& other) PRIME_NOEXCEPT
    {
        _pointer = other.detach();
    }

    template <typename Other>
    ScopedPtr(ScopedPtr<Other>&& other) PRIME_NOEXCEPT
    {
        _pointer = other.detach();
    }

#endif

    ~ScopedPtr()
    {
        reset();
    }

    /// Get the pointer, returns null if no pointer has been assigned.
    Type* get() const { return _pointer; }

    /// Return our pointer.
    operator Type*() const { return _pointer; }

    Type* operator->() const { return _pointer; }

    Type& operator*() const { return *_pointer; }

    bool isNull() const { return _pointer == NULL; }

    /// Detach the pointer from this object and return it.
    Type* detach()
    {
        Type* temp = _pointer;
        _pointer = NULL;
        return temp;
    }

    /// Attach a pointer to this object. If this object already has a pointer, it will be deleted.
    void reset(Type* attach = NULL)
    {
        if (attach != _pointer) {
            Type* old = _pointer;

            _pointer = attach;

            if (old) {
                Deleter::deletePointer(old);
            }
        }
    }

    template <typename Other>
    ScopedPtr& move(ScopedPtr<Other>& other)
    {
        reset();

        _pointer = other.detach();
        return *this;
    }

    template <typename Other>
    ScopedPtr& swap(ScopedPtr<Other>& other)
    {
        Type* temp = _pointer;
        _pointer = other.detach();
        other.reset(temp);
        return *this;
    }

#ifdef PRIME_COMPILER_RVALUEREF

    ScopedPtr& operator=(ScopedPtr&& other) PRIME_NOEXCEPT
    {
        return move(other);
    }

    template <typename Other>
    ScopedPtr& operator=(ScopedPtr<Other>&& other) PRIME_NOEXCEPT
    {
        return move<Other>(other);
    }

#endif

protected:
    Type* _pointer;

private:
    PRIME_UNCOPYABLE(ScopedPtr);
};

//
// ScopedPtrArrayDeleter
//

template <typename Type>
class ScopedPtrArrayDeleter {
public:
    static void deletePointer(Type* pointer) { delete[] pointer; }
};

//
// ScopedArrayPtr
//

/// A std::auto_ptr for arrays. Differs from std::auto_ptr in that you must explicitly pass ownership of the
/// pointer between ScopedArrayPtr instances, e.g., auto_ptr1.move(auto_ptr2);
template <typename Type>
class ScopedArrayPtr : public ScopedPtr<Type, ScopedPtrArrayDeleter<Type>> {
public:
    typedef ScopedPtr<Type, ScopedPtrArrayDeleter<Type>> Super;

    /// Optionally assign the array to be deleted.
    explicit ScopedArrayPtr(Type* assign = NULL)
        : Super(assign)
    {
    }

#ifdef PRIME_COMPILER_RVALUEREF

    ScopedArrayPtr(ScopedArrayPtr&& other) PRIME_NOEXCEPT : Super(other.detach())
    {
    }

    template <typename Other>
    ScopedArrayPtr(ScopedArrayPtr<Other>&& other) PRIME_NOEXCEPT : Super(other.detach())
    {
    }

#endif

    template <typename Index>
    Type& operator[](Index index) const
    {
        return Super::_pointer[index];
    }

#ifdef PRIME_COMPILER_RVALUEREF

    ScopedArrayPtr& operator=(ScopedArrayPtr&& other) PRIME_NOEXCEPT
    {
        return Super::move(other);
    }

    template <typename Other>
    ScopedArrayPtr& operator=(ScopedArrayPtr<Other>&& other) PRIME_NOEXCEPT
    {
        return Super::template move<Other>(other);
    }

#endif

    PRIME_UNCOPYABLE(ScopedArrayPtr);
};
}

#endif
