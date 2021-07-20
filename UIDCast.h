// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UIDCAST_H
#define PRIME_UIDCAST_H

#include "UID.h"

namespace Prime {

//
// UID Casting
//
// Provides an alternative to dynamic_cast that can be enabled for specific classes.
//
// e.g., in the class declaration:
//
//      class MyClass : public SuperClass {
//          PRIME_DECLARE_UID_CAST(SuperClass, 0x82c93d89, 0xda404c81, 0xac3fcb71, 0xda59a5ab)
//
//      public:
//          ...
//
// (you can generate UIDs using the Python script in the comments for the UID class), then, in the
// implementation file:
//
//     PRIME_DEFINE_UID_CAST(MyClass)
//
// (For base classes, use PRIME_DECLARE_UID_CAST_BASE and PRIME_DEFINE_UID_CAST_BASE respectively.)
//
// You can then do:
//
//     void DoSomething(MyClass* mc)
//     {
//         if (DerivedClass* dc = UIDCast<DerivedClass>(mc)) {
//             // mc is indeed a DerivedClass
//
// To support multiple inheritance, still use PRIME_DECLARE_UID_CAST / _BASE in the header but use
// PRIME_BEGIN_DEFINE_UID_CAST(MyClass), PRIME_UID_CAST_SUPER(SuperClass) and PRIME_END_DEFINE_UID_CAST()
// in the implementation file:
//
//     PRIME_BEGIN_DEFINE_UID_CAST(MyClass)
//         PRIME_UID_CAST_SUPER(SuperClass1)
//         PRIME_UID_CAST_SUPER(SuperClass2)
//     PRIME_END_DEFINE_UID_CAST()
//
// A class also gains these members:
//
//     private:
//         typedef superClass Super;
//     public:
//         static const UID classGetUID();
//         virtual UID getClassUID() const;
//         virtual const void* castUID(const UID& to) const;
//

#define PRIME_CLASS_UID(a, b, c, d) \
    static const UID classGetUID() { return UID((a), (b), (c), (d)); }

#define PRIME_DECLARE_UID_CAST(superClass, a, b, c, d) \
    PRIME_SUPER(superClass)                            \
public:                                                \
    PRIME_CLASS_UID((a), (b), (c), (d))                \
    virtual UID getClassUID() const PRIME_OVERRIDE;    \
    virtual const void* castUID(const UID& to) const PRIME_OVERRIDE;

#define PRIME_DECLARE_UID_CAST_BASE(a, b, c, d) \
public:                                         \
    PRIME_CLASS_UID((a), (b), (c), (d))         \
    virtual UID getClassUID() const;            \
    virtual const void* castUID(const UID& to) const;

#define PRIME_BEGIN_DEFINE_UID_CAST(className)          \
    UID className::getClassUID() const                  \
    {                                                   \
        return className::classGetUID();                \
    }                                                   \
                                                        \
    const void* className::castUID(const UID& to) const \
    {                                                   \
        if (to == className::classGetUID()) {           \
            return reinterpret_cast<const void*>(this); \
        }

#define PRIME_UID_CAST_SUPER(superClass)                  \
    {                                                     \
        if (const void* cast = superClass::castUID(to)) { \
            return cast;                                  \
        }                                                 \
    }

#define PRIME_END_DEFINE_UID_CAST() \
    return NULL;                    \
    }

#define PRIME_DEFINE_UID_CAST(className)   \
    PRIME_BEGIN_DEFINE_UID_CAST(className) \
    return Super::castUID(to);             \
    }

#define PRIME_DEFINE_UID_CAST_BASE(className) \
    PRIME_BEGIN_DEFINE_UID_CAST(className)    \
    PRIME_END_DEFINE_UID_CAST()

// This is intended for use in template classes.
#define PRIME_DECLARE_UID_CAST_INLINE(superClass, a, b, c, d) \
public:                                                       \
    typedef superClass Super;                                 \
    PRIME_CLASS_UID((a), (b), (c), (d))                       \
    virtual UID getClassUID() const PRIME_OVERRIDE            \
    {                                                         \
        return classGetUID();                                 \
    }                                                         \
    virtual void* castUID(const UID& to) const PRIME_OVERRIDE \
    {                                                         \
        if (to == classGetUID()) {                            \
            return reinterpret_cast<const void*>(this);       \
        }                                                     \
        return Super::castUID(to);                            \
    }

/// Note that you must do UIDCast<const AnotherType>, not UIDCast<const AnotherType*>.
template <typename Dest, typename Source>
inline Dest* UIDCast(const Source* source)
{
    if (!source) {
        return NULL;
    }

    const void* cast = source->castUID(Dest::classGetUID());

#if !defined(PRIME_NO_RTTI) && defined(PRIME_DEBUG)
    // Use RTTI in debug builds to make sure UID casting has been set up correctly
    PRIME_ASSERT((cast ? 1 : 0) == (dynamic_cast<Dest*>(source) ? 1 : 0));
#endif

    return reinterpret_cast<Dest*>(cast);
}

/// Note that you must do UIDCast<AnotherType>, not UIDCast<AnotherType*>.
template <typename Dest, typename Source>
inline Dest* UIDCast(Source* source)
{
    if (!source) {
        return NULL;
    }

    const void* cast = source->castUID(Dest::classGetUID());

#if !defined(PRIME_NO_RTTI) && defined(PRIME_DEBUG)
    // Use RTTI in debug builds to make sure UID casting has been set up correctly
    PRIME_ASSERT((cast ? 1 : 0) == (dynamic_cast<const Dest*>(source) ? 1 : 0));
#endif

    return reinterpret_cast<Dest*>(const_cast<void*>(cast));
}

/// Note that you must do UIDMustCast<const AnotherType>, not UIDMustCast<const AnotherType*>.
template <typename Dest, typename Source>
inline Dest* UIDMustCast(const Source* source)
{
    PRIME_ASSERT(source);

    const void* cast = source->castUID(Dest::classGetUID());

    PRIME_ASSERT(cast);

    return reinterpret_cast<Dest*>(cast);
}

/// Note that you must do UIDMustCast<AnotherType>, not UIDMustCast<AnotherType*>.
template <typename Dest, typename Source>
inline Dest* UIDMustCast(Source* source)
{
    PRIME_ASSERT(source);

    const void* cast = source->castUID(Dest::classGetUID());

    PRIME_ASSERT(cast);

    return reinterpret_cast<Dest*>(const_cast<void*>(cast));
}
}

#endif
