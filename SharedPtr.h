// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SHAREDPTR_H
#define PRIME_SHAREDPTR_H

#include "Config.h"

#if defined(PRIME_CXX11_STL)

//
// Use C++11 shared_ptr under C++11
//

#include <memory>

namespace Prime {

const std::nullptr_t nullSharedPtr = nullptr;

template <class T>
using SharedPtr = std::template shared_ptr<T>;

template <class T, class... Args>
inline std::shared_ptr<T> MakeShared(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <class T>
using WeakPtr = std::template weak_ptr<T>;
}

#else

//
// Use our own SharedPtr/WeakPtr
//

#include "RefCounting.h" // For atomic operations
#include "ScopedPtr.h"

namespace Prime {

namespace SharedPtrPrivate {
    /// @private
    template <typename Counter>
    class Counts {
    public:
        Counter strong;
        Counter weak;

        Counts()
            : strong(1)
            , weak(1)
        {
        }

        virtual ~Counts() { }

        virtual void deleteObject() PRIME_NOEXCEPT = 0;

        PRIME_UNCOPYABLE(Counts);
    };

    /// @private
    template <typename Type, typename Counter>
    class DeletingCounts : public Counts<Counter> {
    public:
        explicit DeletingCounts(Type* pointer) PRIME_NOEXCEPT : _pointer(pointer)
        {
        }

        virtual void deleteObject() PRIME_OVERRIDE PRIME_NOEXCEPT
        {
            delete _pointer;
        }

    private:
        Type* _pointer;
    };
}

//
// SharedPtr
//

enum NullSharedPtr {
    nullSharedPtr
};

template <typename Type, typename Counter = AtomicCounter>
class SharedPtr {
public:
    typedef SharedPtrPrivate::Counts<Counter> Counts;

    SharedPtr() PRIME_NOEXCEPT : _pointer(NULL),
                                 _counts(NULL)
    {
    }

    /// Assign the pointer.
    explicit SharedPtr(Type* attachPointer)
        : _pointer(attachPointer)
    {
        _counts = new SharedPtrPrivate::DeletingCounts<Type, Counter>(_pointer);
    }

    SharedPtr(NullSharedPtr) PRIME_NOEXCEPT : _pointer(NULL),
                                              _counts(NULL)
    {
    }

    /// Copy constructor. We share the pointer with "copy".
    SharedPtr(const SharedPtr& copy) PRIME_NOEXCEPT : _pointer(copy._pointer),
                                                      _counts(copy._counts)
    {
        if (_counts) {
            _counts->strong.increment();
        }
    }

#ifndef PRIME_COMPILER_NO_MEMBER_TEMPLATE_OVERRIDES
    /// Copy constructor. We share the pointer with "copy".
    template <typename Other>
    SharedPtr(const SharedPtr<Other, Counter>& copy) PRIME_NOEXCEPT : _pointer(copy.get()),
                                                                      _counts(copy._getCounts())
    {
        if (_counts) {
            _counts->strong.increment();
        }
    }
#endif

#ifdef PRIME_COMPILER_RVALUEREF
    SharedPtr(SharedPtr&& move) PRIME_NOEXCEPT : _pointer(move._pointer),
                                                 _counts(move._counts)
    {
        move._pointer = NULL;
        move._counts = NULL;
    }

    template <typename Other>
    SharedPtr(SharedPtr<Other, Counter>&& move) PRIME_NOEXCEPT : _pointer(move.get()),
                                                                 _counts(move._getCounts())
    {
        move._pointer = NULL;
        move._counts = NULL;
    }
#endif

    SharedPtr(Type* pointer, SharedPtrPrivate::Counts<Counter>* counts) PRIME_NOEXCEPT : _pointer(pointer),
                                                                                         _counts(counts)
    {
    }

    ~SharedPtr() { decref(); }

    /// Release the object we're currently sharing and share "copy"s pointer.
    SharedPtr& operator=(const SharedPtr& copy) PRIME_NOEXCEPT
    {
        set(copy._pointer, copy._counts);
        return *this;
    }

/// Release the object we're currently sharing and share "copy"s pointer.
#ifndef PRIME_COMPILER_NO_MEMBER_TEMPLATE_OVERRIDES
    template <typename Other>
    SharedPtr& operator=(const SharedPtr<Other, Counter>& copy) PRIME_NOEXCEPT
    {
        set(copy.get(), copy._getCounts());
        return *this;
    }
#endif

#ifdef PRIME_COMPILER_RVALUEREF
    SharedPtr& operator=(SharedPtr&& from) PRIME_NOEXCEPT
    {
        move(from);
        return *this;
    }

    template <typename Other>
    SharedPtr& operator=(SharedPtr<Other, Counter>&& from) PRIME_NOEXCEPT
    {
        move<Other>(from);
        return *this;
    }
#endif

    /// Release the pointer we're sharing.
    void reset() PRIME_NOEXCEPT
    {
        _reset(NULL, NULL);
    }

    /// There is no way to assign a pointer since that could be used to circumvent the reference counting.
    void reset(Type* pointer) PRIME_NOEXCEPT
    {
        _reset(pointer, new SharedPtrPrivate::DeletingCounts<Type, Counter>(pointer));
    }

    void _reset(Type* pointer, SharedPtrPrivate::Counts<Counter>* counts) PRIME_NOEXCEPT
    {
        decref();

        _pointer = pointer;
        _counts = counts;
    }

    /// Dereference operator.
    Type& operator*() const PRIME_NOEXCEPT { return *_pointer; }

    /// Member accessor.
    Type* operator->() const PRIME_NOEXCEPT { return _pointer; }

    bool operator==(const Type* compare) const PRIME_NOEXCEPT { return _pointer == compare; }

    template <typename Other>
    bool operator==(const SharedPtr<Other, Counter>& compare) const PRIME_NOEXCEPT { return _pointer == compare.get(); }

    bool operator!=(const Type* compare) const PRIME_NOEXCEPT { return _pointer != compare; }

    template <typename Other>
    bool operator!=(const SharedPtr<Other, Counter>& compare) const PRIME_NOEXCEPT { return _pointer != compare.get(); }

    /// Returns a count of the number of SharedPtr objects that are sharing the same pointer.
    int getRefCount() const PRIME_NOEXCEPT { return _counts ? (int)_counts->strong.get() : 0; }

    /// Return our pointer.
    Type* get() const PRIME_NOEXCEPT { return _pointer; }

    /// Return our pointer.
    operator Type*() const PRIME_NOEXCEPT { return _pointer; }

    Counts* _getCounts() const PRIME_NOEXCEPT { return const_cast<Counts*>(_counts); }

protected:
    Type* _pointer;

    /// Set our object and reference count.
    void set(Type* pointer, Counts* counts) PRIME_NOEXCEPT
    {
        if (pointer == _pointer) {
            return;
        }

        if (counts) {
            counts->strong.increment();
        }

        decref();

        _counts = counts;
        _pointer = pointer;
    }

    void move(SharedPtr& from) PRIME_NOEXCEPT
    {
        if (_pointer != from._pointer) {
            decref();
            _pointer = from._pointer;
            _counts = from._counts;
            from._pointer = NULL;
            from._counts = NULL;
        }
    }

#ifndef PRIME_COMPILER_NO_MEMBER_TEMPLATE_OVERRIDES
    template <typename Other>
    void move(SharedPtr<Other, Counter>& from) PRIME_NOEXCEPT
    {
        if (_pointer != from._pointer) {
            decref();
            _pointer = from._pointer;
            _counts = from._counts;
            from._pointer = NULL;
            from._counts = NULL;
        }
    }
#endif

private:
    void decref() PRIME_NOEXCEPT
    {
        if (_counts && _counts->strong.decrement() == 0) {
            _counts->deleteObject();
            if (_counts->weak.decrement() == 0) {
                delete _counts;
            }
        }
    }

    Counts* _counts;
};

#ifdef PRIME_HAVE_INCREMENTIFNOTZERO

//
// WeakPtr
//

template <typename Type, typename Counter = AtomicCounter>
class WeakPtr {
public:
    typedef SharedPtrPrivate::Counts<Counter> Counts;

    WeakPtr() PRIME_NOEXCEPT : _pointer(NULL),
                               _counts(NULL)
    {
    }

    ~WeakPtr() PRIME_NOEXCEPT
    {
        decref();
    }

    WeakPtr(const WeakPtr& copy) PRIME_NOEXCEPT : _pointer(copy._pointer),
                                                  _counts(copy._counts)
    {
        if (_counts) {
            _counts->weak.increment();
        }
    }

#ifndef PRIME_COMPILER_NO_MEMBER_TEMPLATE_OVERRIDES
    template <typename Other>
    WeakPtr(const WeakPtr<Other, Counter>& copy) PRIME_NOEXCEPT : _pointer(copy._getPointer()),
                                                                  _counts(copy._getCounts())
    {
        if (_counts) {
            _counts->weak.increment();
        }
    }

    template <typename Other>
    WeakPtr(const SharedPtr<Other, Counter>& copy) PRIME_NOEXCEPT : _pointer(copy.get()),
                                                                    _counts(copy._getCounts())
    {
        if (_counts) {
            _counts->weak.increment();
        }
    }
#endif

    WeakPtr& operator=(const WeakPtr& copy) PRIME_NOEXCEPT
    {
        if (_counts != copy._counts) {
            decref();

            _pointer = copy._pointer;
            _counts = copy._counts;

            if (_counts) {
                _counts->weak.increment();
            }
        }

        return *this;
    }

#ifndef PRIME_COMPILER_NO_MEMBER_TEMPLATE_OVERRIDES
    template <typename Other>
    WeakPtr& operator=(const WeakPtr<Other, Counter>& copy) PRIME_NOEXCEPT
    {
        if (this != reinterpret_cast<const WeakPtr*>(&copy)) {
            decref();

            _pointer = copy._getPointer();
            _counts = copy._getCounts();

            if (_counts) {
                _counts->weak.increment();
            }
        }

        return *this;
    }
#endif

#ifdef PRIME_COMPILER_RVALUEREF

    WeakPtr(WeakPtr&& rhs) PRIME_NOEXCEPT : _pointer(rhs._pointer),
                                            _counts(rhs._detachCounts())
    {
    }

    template <typename Other>
    WeakPtr(WeakPtr<Other, Counter>&& rhs) PRIME_NOEXCEPT : _pointer(rhs._getPointer()),
                                                            _counts(rhs._detachCounts())
    {
    }

    WeakPtr& operator=(WeakPtr&& rhs) PRIME_NOEXCEPT
    {
        if (_counts != rhs._counts) {
            decref();
            _pointer = rhs._getPointer();
            _counts = rhs._detachCounts();
        }

        return *this;
    }

    template <typename Other>
    WeakPtr& operator=(WeakPtr<Other, Counter>&& rhs) PRIME_NOEXCEPT
    {
        if (this != reinterpret_cast<const WeakPtr*>(&rhs)) {
            decref();
            _pointer = rhs._getPointer();
            _counts = rhs._detachCounts();
        }

        return *this;
    }

#endif

    void reset() PRIME_NOEXCEPT
    {
        decref();
        _counts = NULL;
    }

    SharedPtr<Type, Counter> lock() PRIME_NOEXCEPT
    {
        SharedPtr<Type, Counter> ptr;

        if (_counts && _counts->strong.incrementIfNotZero()) {
            ptr._reset(_pointer, _counts);
        }

        return ptr;
    }

    bool expired() const PRIME_NOEXCEPT
    {
        if (!_counts) {
            return false;
        }

        return _counts->strong.get() == 0;
    }

    Counts* _getCounts() const PRIME_NOEXCEPT { return const_cast<Counts*>(_counts); }

    Counts* _detachCounts() PRIME_NOEXCEPT
    {
        Counts* counts = _counts;
        _counts = NULL;
        return counts;
    }

    Type* _getPointer() const PRIME_NOEXCEPT { return _pointer; }

private:
    /// The caller must reset _counts and _pointer after calling this.
    void decref() PRIME_NOEXCEPT
    {
        if (_counts && _counts->weak.decrement() == 0) {
            delete _counts;
        }
    }

    Counts* _counts;
    Type* _pointer;
};

#endif

//
// MakeShared
//

namespace SharedPtrPrivate {

    /// @private
    template <typename Type, typename Counter = AtomicCounter>
    class MakeSharedBundle : public Counts<Counter> {
    public:
        union {
            PRIME_ALIGNED_TYPE(PRIME_ALIGNOF(Type))
            align;
            char raw[sizeof(Type)];
        } _union;

        ~MakeSharedBundle() { }

        virtual void deleteObject() PRIME_OVERRIDE
        {
            reinterpret_cast<Type*>(_union.raw)->~Type();
        }
    };
}

#ifdef PRIME_COMPILER_RVALUEREF

template <typename Type, typename... Args>
SharedPtr<Type> MakeShared(Args&&... args)
{
    ScopedPtr<SharedPtrPrivate::MakeSharedBundle<Type>> bundle(new SharedPtrPrivate::MakeSharedBundle<Type>());
    Type* object = new (bundle->_union.raw) Type(std::forward<Args>(args)...);
    return SharedPtr<Type>(object, bundle.detach());
}

#else

// Without perfect forwarding, pass-by-value becomes pass-by-reference, potentially incurring a copy.

template <typename Type>
inline SharedPtr<Type> MakeShared()
{
    ScopedPtr<SharedPtrPrivate::MakeSharedBundle<Type>> bundle(new SharedPtrPrivate::MakeSharedBundle<Type>());
    Type* object = new (bundle->_union.raw) Type();
    return SharedPtr<Type>(object, bundle.detach());
}

template <typename Type, typename A0>
inline SharedPtr<Type> MakeShared(const A0& a0)
{
    ScopedPtr<SharedPtrPrivate::MakeSharedBundle<Type>> bundle(new SharedPtrPrivate::MakeSharedBundle<Type>());
    Type* object = new (bundle->_union.raw) Type(a0);
    return SharedPtr<Type>(object, bundle.detach());
}

template <typename Type, typename A0, typename A1>
inline SharedPtr<Type> MakeShared(const A0& a0, const A1& a1)
{
    ScopedPtr<SharedPtrPrivate::MakeSharedBundle<Type>> bundle(new SharedPtrPrivate::MakeSharedBundle<Type>());
    Type* object = new (bundle->_union.raw) Type(a0, a1);
    return SharedPtr<Type>(object, bundle.detach());
}

template <typename Type, typename A0, typename A1, typename A2>
inline SharedPtr<Type> MakeShared(const A0& a0, const A1& a1, const A2& a2)
{
    ScopedPtr<SharedPtrPrivate::MakeSharedBundle<Type>> bundle(new SharedPtrPrivate::MakeSharedBundle<Type>());
    Type* object = new (bundle->_union.raw) Type(a0, a1, a2);
    return SharedPtr<Type>(object, bundle.detach());
}

template <typename Type, typename A0, typename A1, typename A2, typename A3>
inline SharedPtr<Type> MakeShared(const A0& a0, const A1& a1, const A2& a2, const A3& a3)
{
    ScopedPtr<SharedPtrPrivate::MakeSharedBundle<Type>> bundle(new SharedPtrPrivate::MakeSharedBundle<Type>());
    Type* object = new (bundle->_union.raw) Type(a0, a1, a2, a3);
    return SharedPtr<Type>(object, bundle.detach());
}

template <typename Type, typename A0, typename A1, typename A2, typename A3, typename A4>
inline SharedPtr<Type> MakeShared(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4)
{
    ScopedPtr<SharedPtrPrivate::MakeSharedBundle<Type>> bundle(new SharedPtrPrivate::MakeSharedBundle<Type>());
    Type* object = new (bundle->_union.raw) Type(a0, a1, a2, a3, a4);
    return SharedPtr<Type>(object, bundle.detach());
}

template <typename Type, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
inline SharedPtr<Type> MakeShared(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5)
{
    ScopedPtr<SharedPtrPrivate::MakeSharedBundle<Type>> bundle(new SharedPtrPrivate::MakeSharedBundle<Type>());
    Type* object = new (bundle->_union.raw) Type(a0, a1, a2, a3, a4, a5);
    return SharedPtr<Type>(object, bundle.detach());
}

template <typename Type, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
inline SharedPtr<Type> MakeShared(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6)
{
    ScopedPtr<SharedPtrPrivate::MakeSharedBundle<Type>> bundle(new SharedPtrPrivate::MakeSharedBundle<Type>());
    Type* object = new (bundle->_union.raw) Type(a0, a1, a2, a3, a4, a5, a6);
    return SharedPtr<Type>(object, bundle.detach());
}

template <typename Type, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
inline SharedPtr<Type> MakeShared(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7)
{
    ScopedPtr<SharedPtrPrivate::MakeSharedBundle<Type>> bundle(new SharedPtrPrivate::MakeSharedBundle<Type>());
    Type* object = new (bundle->_union.raw) Type(a0, a1, a2, a3, a4, a5, a6, a7);
    return SharedPtr<Type>(object, bundle.detach());
}

template <typename Type, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
inline SharedPtr<Type> MakeShared(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5, const A6& a6, const A7& a7, const A8& a8)
{
    ScopedPtr<SharedPtrPrivate::MakeSharedBundle<Type>> bundle(new SharedPtrPrivate::MakeSharedBundle<Type>());
    Type* object = new (bundle->_union.raw) Type(a0, a1, a2, a3, a4, a5, a6, a7, a8);
    return SharedPtr<Type>(object, bundle.detach());
}

#endif
}

#endif // C++11

#endif
