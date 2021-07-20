// Copyright 2000-2021 Mark H. P. Lord

// Thread-safe atomic integers, RefCounted and RefPtr

#ifndef PRIME_REFCOUNTED_H
#define PRIME_REFCOUNTED_H

#include "Config.h"

//
// Thread-safe atomic operations. The atomic operations aren't always memory barriers, so you must call
// AtomicCounter::atomicFullBarrier() if you need one.
//

#if defined(PRIME_NO_THREADS)

#define PRIME_NONATOMICREFCOUNTING

#elif defined(PRIME_COMPILER_CXX11_ATOMICS)

//
// C++11 atomics
//

#include <atomic>

namespace Prime {
namespace AtomicCounterPrivate {

    typedef int32_t Value;
    typedef std::atomic<Value> Object;

    inline Value Increment(Object* rc) PRIME_NOEXCEPT
    {
        return ++(*rc);
    }

    inline Value Or(Object* rc, Value v) PRIME_NOEXCEPT
    {
        return *rc |= v;
    }

#define PRIME_HAVE_INCREMENTIFNOTZERO
    inline Value IncrementIfNotZero(Object* rc) PRIME_NOEXCEPT
    {
        Value was = rc->load(std::memory_order_relaxed);
        while (was) {
            if (rc->compare_exchange_weak(was, was + 1)) {
                was += 1;
                break;
            }

            was = rc->load(std::memory_order_relaxed);
        }
        return was;
    }

    inline Value Decrement(Object* rc) PRIME_NOEXCEPT
    {
        return --(*rc);
    }

    inline Value Add(Object* rc, Value second) PRIME_NOEXCEPT
    {
        return rc->fetch_add(second) + second;
    }

    inline Value Read(Object* rc) PRIME_NOEXCEPT
    {
        return rc->load();
    }

    inline void Write(Object* rc, Value newValue) PRIME_NOEXCEPT
    {
        rc->store(newValue);
    }

    inline void FullBarrier() PRIME_NOEXCEPT
    {
    }

    inline void AtomicFullBarrier() PRIME_NOEXCEPT
    {
    }
}
}

#elif defined(PRIME_OS_PS3_PPU)

//
// PS3
//

#include <ppu_intrinsics.h>

namespace Prime {
namespace AtomicCounterPrivate {

    typedef int32_t Value;
    typedef volatile Value Object;

    inline Value Increment(Object* rc) PRIME_NOEXCEPT
    {
        Value oldValue;

        do {
            oldValue = __lwarx(rc);
        } while (!__stwcx(rc, oldValue + 1));

        return oldValue + 1;
    }

    inline Value Decrement(Object* rc) PRIME_NOEXCEPT
    {
        Value oldValue;

        do {
            oldValue = __lwarx(rc);
        } while (!__stwcx(rc, oldValue - 1));

        return oldValue - 1;
    }

    inline Value Add(Object* rc, Value second) PRIME_NOEXCEPT
    {
        Value oldValue;

        do {
            oldValue = __lwarx(rc);
        } while (!__stwcx(rc, oldValue + second));

        return oldValue + second;
    }

    inline Value Or(Object* rc, Value second) PRIME_NOEXCEPT
    {
        Value oldValue;

        do {
            oldValue = __lwarx(rc);
        } while (!__stwcx(rc, oldValue | second));

        return oldValue | second;
    }

    inline Value Read(Object* rc) PRIME_NOEXCEPT
    {
        Value oldValue;

        do {
            oldValue = __lwarx(rc);
        } while (!__stwcx(rc, oldValue));

        return oldValue;
    }

    inline void Write(Object* rc, Value newValue) PRIME_NOEXCEPT
    {
        // This seems idiomatic according to the IBM docs.
        do {
            (void)__lwarx(rc);
        } while (!__stwcx(rc, newValue));
    }

    inline void FullBarrier() PRIME_NOEXCEPT
    {
        __lwsync();
    }

    /// On platforms where atomic operations aren't full barriers, this is a full barrier. Otherwise, it's a no-op.
    inline void AtomicFullBarrier() PRIME_NOEXCEPT
    {
        FullBarrier();
    }
}
}

#elif defined(_XBOX_VER) || defined(PRIME_OS_XBOX)

//
// Xbox/360
//

// Using assembly for atomics is discouraged by Microsoft, so we need xtl.h
#include <xtl.h>

#pragma warning(push)
#pragma warning(disable : 4100) // warning about unused arguments

namespace Prime {
namespace AtomicCounterPrivate {

    typedef long Value;
    typedef volatile Value Object;

    inline Value Increment(Object* rc) PRIME_NOEXCEPT
    {
        return _InterlockedIncrement(rc);
    }

#define PRIME_HAVE_INCREMENTIFNOTZERO
    inline Value IncrementIfNotZero(Object* rc) PRIME_NOEXCEPT
    {
        Value was = _InterlockedExchangeAdd(rc, 0);
        while (was) {
            if (_InterlockedCompareExchange(rc, was + 1, was) == was) {
                was += 1;
                break;
            }

            was = *rc;
        }
        return was;
    }

    inline Value Decrement(Object* rc) PRIME_NOEXCEPT
    {
        return _InterlockedDecrement(rc);
    }

    inline Value Add(Object* rc, Value second) PRIME_NOEXCEPT
    {
        return _InterlockedExchangeAdd(rc, second) + second;
    }

    inline Value Or(Object* rc, Value second) PRIME_NOEXCEPT
    {
        for (;;) {
            Value was = *rc;

            if (_InterlockedCompareExchange(rc, was | second, was) == was) {
                return was | second;
            }
        }
    }

    inline Value Read(Object* rc) PRIME_NOEXCEPT
    {
        return _InterlockedExchangeAdd(rc, 0);
    }

    inline void Write(Object* rc, Value newValue) PRIME_NOEXCEPT
    {
        _InterlockedExchange(rc, newValue);
    }

    inline void FullBarrier() PRIME_NOEXCEPT
    {
        __lwsync();
    }

    /// On platforms where atomic operations aren't full barriers, this is a full barrier. Otherwise, it's a no-op.
    inline void AtomicFullBarrier() PRIME_NOEXCEPT
    {
        // 360's _InterlockedXXX functions aren't barriers (Windows' are).
        FullBarrier();
    }
}
}

#pragma warning(pop)

#elif PRIME_MSC_AND_NEWER(1310)

//
// Visual C++ intrinsics
//

#if _MSC_VER < 1600
// Due to an issue in VC++ 2008 x64, you need to include math.h before intrin.h.
#include <math.h>
#endif
#include <intrin.h>

#pragma warning(push)
#pragma warning(disable : 4100) // warning about unused arguments

namespace Prime {
namespace AtomicCounterPrivate {

    typedef long Value;
    typedef volatile Value Object;

    // int32_t is often an int but _InterlockedIncrement/decrement ask for a long.
    PRIME_COMPILE_TIME_ASSERT(sizeof(Value) == 4);

    inline Value Increment(Object* rc) PRIME_NOEXCEPT
    {
        return _InterlockedIncrement(rc);
    }

#define PRIME_HAVE_INCREMENTIFNOTZERO
    inline Value IncrementIfNotZero(Object* rc) PRIME_NOEXCEPT
    {
        Value was = _InterlockedExchangeAdd(rc, 0);
        while (was) {
            if (_InterlockedCompareExchange(rc, was + 1, was) == was) {
                was += 1;
                break;
            }

            was = *rc;
        }
        return was;
    }

    inline Value Decrement(Object* rc) PRIME_NOEXCEPT
    {
        return _InterlockedDecrement(rc);
    }

    inline Value Add(Object* rc, Value second) PRIME_NOEXCEPT
    {
        return _InterlockedExchangeAdd(rc, second) + second;
    }

    inline Value Or(Object* rc, Value second) PRIME_NOEXCEPT
    {
        for (;;) {
            Value was = *rc;

            if (_InterlockedCompareExchange(rc, was | second, was) == was) {
                return was | second;
            }
        }
    }

    inline Value Read(Object* rc) PRIME_NOEXCEPT
    {
        return _InterlockedExchangeAdd(rc, 0);
    }

    inline void Write(Object* rc, Value newValue) PRIME_NOEXCEPT
    {
        _InterlockedExchange(rc, newValue);
    }

    inline void FullBarrier() PRIME_NOEXCEPT
    {
        // Requires SSE2
        _mm_mfence();
    }

    /// On platforms where atomic operations aren't full barriers, this is a full barrier. Otherwise, it's a no-op.
    inline void AtomicFullBarrier() PRIME_NOEXCEPT
    {
    }
}
}

#pragma warning(pop)

#elif defined(PRIME_OS_WINDOWS)

//
// Windows SDK
//

#include "Windows/WindowsConfig.h"

#pragma warning(push)
#pragma warning(disable : 4100) // warning about unused arguments

namespace Prime {
namespace AtomicCounterPrivate {

    typedef long Value;
#if WINVER >= 0x0500
    typedef volatile Value Object;
#else
    typedef Value Object;
#endif

    inline Value Increment(Object* rc) PRIME_NOEXCEPT
    {
        return InterlockedIncrement(rc);
    }

    inline Value Decrement(Object* rc) PRIME_NOEXCEPT
    {
        return InterlockedDecrement(rc);
    }

    inline Value Add(Object* rc, Value second) PRIME_NOEXCEPT
    {
        return InterlockedExchangeAdd(rc, second) + second;
    }

#define PRIME_HAVE_INCREMENTIFNOTZERO
    inline Value IncrementIfNotZero(Object* rc) PRIME_NOEXCEPT
    {
        Value was = Add(rc, 0);
        while (was) {
#if PRIME_MSC_AND_OLDER(1300)
            if (InterlockedCompareExchange((void**)rc, (void*)(was + 1), (void*)was) == (void*)was) {
#else
            if (InterlockedCompareExchange(rc, was + 1, was) == was) {
#endif
                was += 1;
                break;
            }

            was = *rc;
        }
        return was;
    }

    inline Value Or(Object* rc, Value second) PRIME_NOEXCEPT
    {
        for (;;) {
            Value was = *rc;

            if (InterlockedCompareExchange((void**)rc, (void*)(was | second), (void*)was) == (void*)was) {
                return was | second;
            }
        }
    }
    inline Value Read(Object* rc) PRIME_NOEXCEPT
    {
        return InterlockedExchangeAdd(rc, 0);
    }

    inline void Write(Object* rc, Value newValue) PRIME_NOEXCEPT
    {
        InterlockedExchange(rc, newValue);
    }

    inline void FullBarrier() PRIME_NOEXCEPT
    {
        // Not available!
    }

    /// On platforms where atomic operations aren't full barriers, this is a full barrier. Otherwise, it's a no-op.
    inline void AtomicFullBarrier() PRIME_NOEXCEPT
    {
    }
}
}

#pragma warning(pop)

#elif PRIME_GCC_AND_NEWER(4, 1, 0) || defined(__clang__) || defined(__SNC__)

//
// GCC intrinsics (emulated by Clang and SNC)
//

namespace Prime {
namespace AtomicCounterPrivate {

    typedef int32_t Value;
    typedef volatile Value Object;

    inline Value Increment(Object* rc) PRIME_NOEXCEPT
    {
        return __sync_add_and_fetch(rc, 1);
    }

#define PRIME_HAVE_INCREMENTIFNOTZERO
    inline Value IncrementIfNotZero(Object* rc) PRIME_NOEXCEPT
    {
        Value was = __sync_add_and_fetch(rc, 0);
        while (was) {
            if (__sync_bool_compare_and_swap(rc, was, was + 1)) {
                was += 1;
                break;
            }

            was = *rc;
        }
        return was;
    }

    inline Value Decrement(Object* rc) PRIME_NOEXCEPT
    {
        return __sync_sub_and_fetch(rc, 1);
    }

    inline Value Add(Object* rc, Value second) PRIME_NOEXCEPT
    {
        return __sync_add_and_fetch(rc, second);
    }

    inline Value Or(Object* rc, Value second) PRIME_NOEXCEPT
    {
        return __sync_or_and_fetch(rc, second);
    }

    inline Value Read(Object* rc) PRIME_NOEXCEPT
    {
        return __sync_add_and_fetch(rc, 0);
    }

    inline void FullBarrier() PRIME_NOEXCEPT
    {
        // __builtin_dmb(); on Vita?
        __sync_synchronize();
        //asm volatile("mfence" ::: "memory"); // ...use this if it is on x86/x64. Requires SSE2.
    }

    inline void Write(Object* rc, Value newValue) PRIME_NOEXCEPT
    {
        *rc = newValue;
        FullBarrier();
    }

    /// On platforms where atomic operations aren't full barriers, this is a full barrier. Otherwise, it's a no-op.
    inline void AtomicFullBarrier() PRIME_NOEXCEPT
    {
    }
}
}

#elif defined(PRIME_OS_OSX)

//
// Darwin libkern
//

// No longer needed thanks to compiler intrinsics
// If this ever does become useful again, it's missing IncrementIfNotZero and Or

#include <libkern/OSAtomic.h>

namespace Prime {
namespace AtomicCounterPrivate {

    typedef int32_t Value;
    typedef volatile Value Object;

    inline Value Increment(Object* rc) PRIME_NOEXCEPT
    {
        return OSAtomicIncrement32(rc);
    }

    inline Value Decrement(Object* rc) PRIME_NOEXCEPT
    {
        return OSAtomicDecrement32(rc);
    }

    inline Value Add(Object* rc, Value second) PRIME_NOEXCEPT
    {
        return OSAtomicAdd32(rc, second);
    }

    inline Value Read(Object* rc) PRIME_NOEXCEPT
    {
        return OSAtomicAdd32(rc, 0);
    }

    inline void Write(Object* rc, Value newValue) PRIME_NOEXCEPT
    {
        *rc = newValue;
        OSMemoryBarrier();
    }

    inline void FullBarrier() PRIME_NOEXCEPT
    {
        OSMemoryBarrier();
    }

    /// On platforms where atomic operations aren't full barriers, this is a full barrier. Otherwise, it's a no-op.
    inline void AtomicFullBarrier() PRIME_NOEXCEPT
    {
#if !defined(PRIME_CPU_ANY_X86)
        FullBarrier();
#endif
    }
}
}

#elif defined(__GNUC__) && (defined(PRIME_CPU_386) || defined(PRIME_CPU_X64))

//
//  GCC x86 inline assembly
//

// No longer needed thanks to compiler intrinsics
// If this ever does become useful again, it's missing IncrementIfNotZero and Or

namespace Prime {
namespace AtomicCounterPrivate {

    typedef int32_t Value;
    typedef volatile Value Object;

    inline Value Increment(Object* rc) PRIME_NOEXCEPT
    {
        Value result;

        asm volatile(
            "lock\n"
            "xaddl %0, %1\n"
            : "=r"(result), "=m"(*rc)
            : "0"(1), "m"(*rc)
            : "memory", "cc");

        return result + 1;
    }

    inline Value Decrement(Object* rc) PRIME_NOEXCEPT
    {
        Value result;

        asm volatile(
            "lock\n"
            "xaddl %0, %1\n"
            : "=r"(result), "=m"(*rc)
            : "0"(-1), "m"(*rc)
            : "memory", "cc");

        return result - 1;
    }

    inline Value Add(Object* rc, Value second) PRIME_NOEXCEPT
    {
        Value result;

        asm volatile(
            "lock\n"
            "xaddl %0, %1\n"
            : "=r"(result), "=m"(*rc)
            : "0"(second), "m"(*rc)
            : "memory", "cc");

        return result + second;
    }

    inline Value Read(Object* rc) PRIME_NOEXCEPT
    {
        return Add(rc, 0);
    }

    inline void FullBarrier() PRIME_NOEXCEPT
    {
        // This is only available on SSE2 and later (anything before Pentium 4 or before and including Athlon 64)
        asm volatile("mfence" ::
                         : "memory");
    }

    inline void Write(Object* rc, Value newValue) PRIME_NOEXCEPT
    {
        *rc = newValue;
        FullBarrier();
    }

    /// On platforms where atomic operations aren't full barriers, this is a full barrier. Otherwise, it's a no-op.
    inline void AtomicFullBarrier() PRIME_NOEXCEPT
    {
    }
}
}

#elif defined(__GNUC__) && defined(PRIME_CPU_PPC)

//
// GCC PowerPC inline assembly
//

// No longer needed thanks to compiler intrinsics
// If this ever does become useful again, it's missing IncrementIfNotZero and Or

namespace Prime {
namespace AtomicCounterPrivate {

    typedef int32_t Value;
    typedef volatile Value Object;

    inline Value Increment(Object* rc) PRIME_NOEXCEPT
    {
        Value result;

        asm volatile(
            "1: lwarx   %0,0,%1\n"
            "   addic   %0,%0,1\n"
            "   stwcx.  %0,0,%1\n"
            "   bne-    1b"
            : "=&r"(result)
            : "r"(rc)
            : "cc", "memory");

        return result;
    }

    inline Value Decrement(Object* rc) PRIME_NOEXCEPT
    {
        Value result;

        asm volatile(
            "1: lwarx   %0,0,%1\n"
            "   addic   %0,%0,-1\n"
            "   stwcx.  %0,0,%1\n"
            "   bne-    1b"
            : "=&r"(result)
            : "r"(rc)
            : "cc", "memory");

        return result;
    }

    inline Value Add(Object* rc, Value second) PRIME_NOEXCEPT
    {
        Value result;

        asm volatile(
            "1: lwarx   %0,0,%2\n"
            "   add      %0,%1,%0\n"
            "   stwcx.  %0,0,%2\n"
            "   bne-     1b\n"
            : "=&b"(result)
            : "r"(second), "b"(rc)
            : "cc", "memory");

        return result;
    }

    inline Value Read(Object* rc) PRIME_NOEXCEPT
    {
        return Add(rc, 0);
    }

    inline void Write(Object* rc, Value newValue) PRIME_NOEXCEPT
    {
        Value result;

        // Fetch and store example from IBM's documentation.
        asm volatile(
            "1: lwarx   %0,0,%2\n"
            "   lwz      %0,0(%1)\n"
            "   stwcx.  %0,0,%2\n"
            "   bne-     1b\n"
            : "=&b"(result)
            : "r"(newValue), "b"(rc)
            : "cc", "memory");

        return result;
    }

    inline void FullBarrier() PRIME_NOEXCEPT
    {
        asm volatile("lwsync" ::
                         : "memory");
    }

    /// On platforms where atomic operations aren't full barriers, this is a full barrier. Otherwise, it's a no-op.
    inline void AtomicFullBarrier() PRIME_NOEXCEPT
    {
        FullBarrier();
    }
}
}

#else

// If PRIME_NO_THREADS should be defined for a platform then #define it in Platform.h (if threads aren't supported
// by the platform) or Config.h (if threads aren't supported by the project).
#warning "Using non-thread-safe atomic reference counting despite PRIME_NO_THREADS not being defined"

#define PRIME_NONATOMICREFCOUNTING

#endif

#ifdef PRIME_NONATOMICREFCOUNTING

namespace Prime {
namespace AtomicCounterPrivate {

    typedef int32_t Value;
    typedef Value Object;

    inline Value Increment(Object* rc) PRIME_NOEXCEPT
    {
        return ++(*rc);
    }

    inline Value IncrementIfNotZero(Object* rc) PRIME_NOEXCEPT
    {
        if (*rc) {
            ++(*rc);
        }

        return *rc;
    }

    inline Value Decrement(Object* rc) PRIME_NOEXCEPT
    {
        return --(*rc);
    }

    inline Value Add(Object* rc, Value second) PRIME_NOEXCEPT
    {
        (*rc) += second;
        return *rc;
    }

    inline Value Or(Object* rc, Value second) PRIME_NOEXCEPT
    {
        (*rc) |= second;
        return *rc;
    }

    inline Value Read(Object* rc) PRIME_NOEXCEPT
    {
        return *rc;
    }

    inline void Write(Object* rc, Value newValue) PRIME_NOEXCEPT
    {
        *rc = newValue;
    }

    inline void FullBarrier() PRIME_NOEXCEPT
    {
    }

    /// On platforms where atomic operations aren't full barriers, this is a full barrier. Otherwise, it's a no-op.
    inline void AtomicFullBarrier() PRIME_NOEXCEPT
    {
    }
}
}

#endif

namespace Prime {

//
// AtomicCounter
//

/// Encapsulate a reference count which is modified in a thread-safe manner.
class AtomicCounter {
public:
    typedef AtomicCounterPrivate::Value Value;

    /// This is a full barrier only if atomic increment/decrement aren't already full barriers.
    static void atomicFullBarrier() PRIME_NOEXCEPT { AtomicCounterPrivate::AtomicFullBarrier(); }

    /// This is always a full barrier.
    static void fullBarrier() PRIME_NOEXCEPT { AtomicCounterPrivate::FullBarrier(); }

    explicit AtomicCounter(Value count) PRIME_NOEXCEPT : _count(count)
    {
    }

    Value increment() PRIME_NOEXCEPT { return AtomicCounterPrivate::Increment(&_count); }

#ifdef PRIME_HAVE_INCREMENTIFNOTZERO
    Value incrementIfNotZero() PRIME_NOEXCEPT
    {
        return AtomicCounterPrivate::IncrementIfNotZero(&_count);
    }
#endif

    Value decrement() PRIME_NOEXCEPT
    {
        return AtomicCounterPrivate::Decrement(&_count);
    }

    Value add(Value second) PRIME_NOEXCEPT { return AtomicCounterPrivate::Add(&_count, second); }

    Value get() const PRIME_NOEXCEPT { return AtomicCounterPrivate::Read(&_count); }

    void set(Value value) PRIME_NOEXCEPT { AtomicCounterPrivate::Write(&_count, value); }

    Value operator++() PRIME_NOEXCEPT { return increment(); }
    Value operator--() PRIME_NOEXCEPT { return decrement(); }
    Value operator++(int) PRIME_NOEXCEPT { return increment() - 1; }
    Value operator--(int) PRIME_NOEXCEPT { return decrement() + 1; }
    Value operator|=(Value value) PRIME_NOEXCEPT { return AtomicCounterPrivate::Or(&_count, value); }

private:
    mutable AtomicCounterPrivate::Object _count;

    PRIME_UNCOPYABLE(AtomicCounter);
};

//
// NonAtomicCounter
//

/// Encapsulate a reference count which is modified in a non-thread-safe manner.
class NonAtomicCounter {
public:
    typedef int Value;

    explicit NonAtomicCounter(Value count) PRIME_NOEXCEPT : _count(count)
    {
    }

    Value increment() PRIME_NOEXCEPT { return ++_count; }

    Value decrement() PRIME_NOEXCEPT { return --_count; }

    Value incrementIfNotZero() PRIME_NOEXCEPT
    {
        if (_count) {
            ++_count;
        }

        return _count;
    }

    Value add(Value second) PRIME_NOEXCEPT { return _count += second; }

    Value get() const PRIME_NOEXCEPT { return _count; }

    static void fullBarrier() PRIME_NOEXCEPT { }

    static void atomicFullBarrier() PRIME_NOEXCEPT { }

    Value operator++() PRIME_NOEXCEPT { return increment(); }
    Value operator--() PRIME_NOEXCEPT { return decrement(); }
    Value operator++(int) PRIME_NOEXCEPT { return increment() - 1; }
    Value operator--(int) PRIME_NOEXCEPT { return decrement() + 1; }
    Value operator|=(Value value) PRIME_NOEXCEPT { return _count |= value; }

private:
    Value _count;

    PRIME_UNCOPYABLE(NonAtomicCounter);
};

//
// RefCounted
//

/// Implements reference counting using an atomic counter. retain() and release() are non-virtual, but released()
/// can be overridden to customise destruction (e.g., to return memory to an object pool). The reference counter
/// is initialised to 1, allowing objects to be safely created on the stack or as a member variable, but meaning
/// you must use MakeRef or PassRef when dynamically constructing an instance.
class PRIME_PUBLIC RefCounted {
public:
    typedef AtomicCounter::Value RefCount;

    RefCounted() PRIME_NOEXCEPT : _counter(1)
    {
    }

    virtual ~RefCounted() PRIME_NOEXCEPT { }

    void retain() const PRIME_NOEXCEPT { _counter.increment(); }

    RefCount release() const PRIME_NOEXCEPT
    {
        RefCount newRefCount = (RefCount)_counter.decrement();
        if (!newRefCount) {
            const_cast<RefCounted*>(this)->released();
        }

        return newRefCount;
    }

    /// The default implementation does a `delete this`. Can be overridden to support object pools.
    virtual void released() PRIME_NOEXCEPT;

    /// For diagnostic purposes.
    RefCount getRefCount() const PRIME_NOEXCEPT { return _counter.get(); }

private:
    mutable AtomicCounter _counter;

    RefCounted(const RefCounted&) PRIME_NOEXCEPT : _counter(1)
    {
    }

    RefCounted& operator=(const RefCounted&) PRIME_NOEXCEPT { return *this; }
};

inline void RefPtrRetain(const RefCounted* rc) PRIME_NOEXCEPT
{
    rc->retain();
}

inline void RefPtrRelease(const RefCounted* rc) PRIME_NOEXCEPT
{
    rc->release();
}

//
// CustomRefCounted
//

/// Implements reference counting for a Type without incurring a vtable, and using a non-thread-safe (non-atomic)
/// reference count by default.
template <typename Type, typename Counter>
class CustomRefCounted {
public:
    typedef typename Counter::Value RefCount;

    CustomRefCounted() PRIME_NOEXCEPT : _counter(1)
    {
    }

    void retain() const PRIME_NOEXCEPT { _counter.increment(); }

    RefCount release() const PRIME_NOEXCEPT
    {
        RefCount newRefCount = (RefCount)_counter.decrement();
        if (!newRefCount) {
            const_cast<Type*>(static_cast<const Type*>(this))->released();
        }

        return newRefCount;
    }

    void released() PRIME_NOEXCEPT
    {
        delete static_cast<Type*>(this);
    }

private:
    mutable Counter _counter;

    CustomRefCounted(CustomRefCounted&)
        : _counter(1)
    {
    }

    CustomRefCounted& operator=(const CustomRefCounted&)
    {
        return *this;
    }
};

template <typename Type, typename Counter>
inline void RefPtrRetain(const CustomRefCounted<Type, Counter>* rc) PRIME_NOEXCEPT
{
    rc->retain();
}

template <typename Type, typename Counter>
inline void RefPtrRelease(const CustomRefCounted<Type, Counter>* rc) PRIME_NOEXCEPT
{
    rc->release();
}

//
// NonAtomicRefCounted
//

/// A fast reference-counting base class for hierarchies where thread safety is not necessary.
template <typename Type>
class NonAtomicRefCounted : public CustomRefCounted<Type, NonAtomicCounter> {
public:
    typedef CustomRefCounted<Type, NonAtomicCounter> Super;
    typedef typename CustomRefCounted<Type, NonAtomicCounter>::RefCount RefCount;

    NonAtomicRefCounted() PRIME_NOEXCEPT
    {
    }

private:
    PRIME_UNCOPYABLE(NonAtomicRefCounted);
};

//
// Utility functions
//

/// If the pointer is non-null, release the object it points to, then reset the pointer to null.
template <typename Type>
inline void SafeRelease(Type*& pointer) PRIME_NOEXCEPT
{
    if (pointer) {
        RefPtrRelease(pointer);
        pointer = NULL;
    }
}

/// Assign rhs to lhs, retaining rhs if not null and releasing lhs if not null.
template <typename Type, typename Other>
inline void SafeAssignRefCounted(Type*& lhs, Other*& rhs) PRIME_NOEXCEPT
{
    if (lhs != rhs) {
        if (rhs) {
            RefPtrRetain(rhs);
        }

        if (lhs) {
            RefPtrRelease(lhs);
        }

        lhs = rhs;
    }
}

/// A smart pointer that retains a reference to any object that has an implementation of RefPtrRetain() and
/// RefPtrRelease() (such as those derived from RefCounted). You can use the PassRef() function to
/// assign an object to a RefPtr without incrementing its reference count e.g.: RefPtr<T> o = PassRef(new T).
/// Note that reference counted objects have an initial count of 1, so you must use PassRef() when constructing a
/// RefPtr with a new object. Type can have a const specifier, e.g., RefPtr<const Log>, which will allow
/// assignment from a RefPtr<Log> or RefPtr<const Log>, but which cannot be assigned to a RefPtr<Log>.
template <typename Type>
class RefPtr {
public:
    RefPtr() PRIME_NOEXCEPT : _object(NULL)
    {
    }

    /// Retains the object. To assign to a RefPtr without retaining the object, use the PassRef() function,
    /// e.g. RefPtr<Object> myObject = PassRef(new Object);
    RefPtr(Type* assign) PRIME_NOEXCEPT : _object(assign)
    {
        if (assign) {
            RefPtrRetain(assign);
        }
    }

    enum DoNotRetainType { DoNotRetain };

    /// Assign an object without retaining it. Use PassRef() instead of calling this constructor.
    RefPtr(DoNotRetainType, Type* assign) PRIME_NOEXCEPT : _object(assign)
    {
    }

    /// Initialise with the same object as another RefPtr instance. The object is retained.
    RefPtr(const RefPtr& copy) PRIME_NOEXCEPT : _object(copy._object)
    {
        if (_object) {
            RefPtrRetain(_object);
        }
    }

/// Initialise with the same object as another RefPtr instance. The object is retained.
#ifndef PRIME_COMPILER_NO_MEMBER_TEMPLATE_OVERRIDES
    template <typename Other>
    RefPtr(const RefPtr<Other>& copy) PRIME_NOEXCEPT : _object(copy.get())
    {
        if (_object) {
            RefPtrRetain(_object);
        }
    }
#endif

#ifdef PRIME_COMPILER_RVALUEREF

    /// Initialise with the same object as another RefPtr instance. The object remains retained.
    RefPtr(RefPtr&& move) PRIME_NOEXCEPT : _object(move.detach())
    {
    }

    /// Initialise with the same object as another RefPtr instance. The object remains retained.
    template <typename Other>
    RefPtr(RefPtr<Other>&& move) PRIME_NOEXCEPT : _object(move.detach())
    {
    }

#endif

    /// Release the object, if we have one attached.
    ~RefPtr() PRIME_NOEXCEPT
    {
        if (_object) {
            RefPtrRelease(_object);
        }
    }

    bool isNull() const PRIME_NOEXCEPT { return _object == NULL; }

    /// Assign the object owned by the specified RefPtr to this RefPtr.
    RefPtr& operator=(const RefPtr& copy) PRIME_NOEXCEPT
    {
        reset(copy.get());
        return *this;
    }

#ifdef PRIME_COMPILER_RVALUEREF

    RefPtr& operator=(RefPtr&& move) PRIME_NOEXCEPT
    {
        if (this == &move) {
            return *this;
        }

        if (_object) {
            RefPtrRelease(_object);
        }

        _object = move.detach();
        return *this;
    }

    template <typename Other>
    RefPtr& operator=(RefPtr<Other>&& move) PRIME_NOEXCEPT
    {
        if (_object == move.get()) {
            return *this;
        }

        if (_object) {
            RefPtrRelease(_object);
        }

        _object = move.detach();
        return *this;
    }

#endif

    RefPtr& operator=(Type* assign) PRIME_NOEXCEPT
    {
        reset(assign);
        return *this;
    }

    /// Release any object we hold then assign us the new object, and retain it.
    void reset(Type* to = NULL) PRIME_NOEXCEPT
    {
        if (to) {
            RefPtrRetain(to);
        }

        if (_object) {
            RefPtrRelease(_object);
        }

        _object = to;
    }

/// Assign the object owned by the specified RefPtr to this RefPtr. The object is retained.
#ifndef PRIME_COMPILER_NO_MEMBER_TEMPLATE_OVERRIDES
    template <typename Other>
    RefPtr& operator=(const RefPtr<Other>& copy) PRIME_NOEXCEPT
    {
        reset(copy.get());
        return *this;
    }
#endif

    /// Detach the object from this RefPtr. Returns the pointer. The object's reference count is not
    /// affected, so the caller takes responsibility for releasing the object.
    Type* detach() PRIME_NOEXCEPT
    {
        Type* temp = _object;
        _object = NULL;
        return temp;
    }

    /// Release our object then detach it from this RefPtr.
    void release() PRIME_NOEXCEPT
    {
        if (_object) {
            RefPtrRelease(_object);
            _object = NULL;
        }
    }

    RefPtr pass() PRIME_NOEXCEPT
    {
        return RefPtr(DoNotRetain, detach());
    }

    Type* operator->() const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(_object);
        return _object;
    }

    Type& operator*() const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(_object);
        return *_object;
    }

    operator Type*() const PRIME_NOEXCEPT { return _object; }

#ifdef PRIME_COMPILER_EXPLICIT_CONVERSION
    explicit operator bool() const
    {
        return _object != NULL;
    }
#endif

    bool operator!() const PRIME_NOEXCEPT
    {
        return !_object;
    }

    /// Retrieve the pointer. Returns null if no object is attached.
    Type* get() const PRIME_NOEXCEPT { return _object; }

private:
    /// Pointer to the object.
    Type* _object;
};

//
// RefPtrStaticCast and RefPtrConstCast
//

template <typename To, typename From>
RefPtr<To> RefPtrStaticCast(const RefPtr<From>& from) PRIME_NOEXCEPT
{
    return RefPtr<To>(static_cast<To*>(from.get()));
}

#ifdef PRIME_COMPILER_RVALUEREF
template <typename To, typename From>
RefPtr<To> RefPtrStaticCast(RefPtr<From>&& from) PRIME_NOEXCEPT
{
    return RefPtr<To>(RefPtr<To>::DoNotRetain, static_cast<To*>(from.detach()));
}
#endif

template <typename To, typename From>
RefPtr<To> RefPtrConstCast(const RefPtr<From>& from) PRIME_NOEXCEPT
{
    return RefPtr<To>(const_cast<To*>(from.get()));
}

#ifdef PRIME_COMPILER_RVALUEREF
template <typename To, typename From>
RefPtr<To> RefPtrConstCast(RefPtr<From>&& from) PRIME_NOEXCEPT
{
    return RefPtr<To>(RefPtr<To>::DoNotRetain, const_cast<To*>(from.detach()));
}
#endif

///
/// PassRef
///
/// Used to assign a pointer, which should not be retained, to a RefPtr. Can also be used to pass a new
/// object to a function that takes a pointer, e.g.: addPhoneBookEntry(PassRef(new PhoneBookEntry));
///
/// If you were to do:
///
///    RefPtr<MyPhoneBookEntry> entry = new MyPhoneBookEntry;
///
/// (where MyPhoneBookEntry is a reference counted object) then you'd get a memory leak. The MyPhoneBookEntry
/// (which has an initial reference count of 1) would be retained and released by the RefPtr but would still
/// hold a reference count of 1.
///
/// To avoid this, you could do:
///    RefPtr<MyPhoneBookEntry> entry = new MyPhoneBookEntry;
///    entry->release(); // Note: not entry.release()
///
/// but, using PassRef, that becomes:
///    RefPtr<MyPhoneBookEntry> entry = PassRef(new MyPhoneBookEntry);
///
template <typename Type>
inline RefPtr<Type> PassRef(Type* object) PRIME_NOEXCEPT
{
    return RefPtr<Type>(RefPtr<Type>::DoNotRetain, object);
}

///
/// MakeRef
///
/// MakeRef<MyPhoneBookEntry>("Mum") is equivalent to PassRef(new MyPhoneBookEntry("Mum"))
/// Requires C++11
///

#ifdef PRIME_COMPILER_VARIADIC_TEMPLATES

template <class Type, class... Args>
inline RefPtr<Type> MakeRef(Args&&... args) PRIME_NOEXCEPT
{
    return RefPtr<Type>(RefPtr<Type>::DoNotRetain, new Type(std::forward<Args>(args)...));
}

#endif

///
/// Ref
///
/// Create a RefPtr.
///
template <typename Type>
inline RefPtr<Type> Ref(Type* object) PRIME_NOEXCEPT
{
    return RefPtr<Type>(object);
}

template <typename Type>
inline const RefPtr<Type>& Ref(const RefPtr<Type>& object) PRIME_NOEXCEPT
{
    return object;
}
}

#endif
