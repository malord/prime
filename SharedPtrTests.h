// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SHAREDPTRTESTS_H
#define PRIME_SHAREDPTRTESTS_H

#include "SharedPtr.h"

namespace Prime {

namespace SharedPtrTestPrivate {

    class Thing {
    public:
        static int alive;

        Thing() { ++alive; }

        ~Thing() { --alive; }
    };

    class Derived : public Thing {
    public:
    };

    int Thing::alive = 0;
}

inline void SharedPtrTests()
{
    using namespace SharedPtrTestPrivate;

    {
        AtomicCounter ac(0);
        PRIME_TEST(ac.incrementIfNotZero() == 0);
        ac.increment();
        PRIME_TEST(ac.incrementIfNotZero() == 2);
    }

    SharedPtr<Derived> z = MakeShared<Derived>();
    SharedPtr<Thing> a = z;
    WeakPtr<Thing> b = a;
    WeakPtr<Thing> c = b;
    WeakPtr<Thing> d = z;

    PRIME_TEST(b.lock() == a);
    PRIME_TEST(c.lock() == a);
    PRIME_TEST(d.lock() == a);
    PRIME_TEST(!b.expired());
    PRIME_TEST(!c.expired());
    PRIME_TEST(!d.expired());

    z.reset();

    PRIME_TEST(b.lock() == a);
    PRIME_TEST(c.lock() == a);
    PRIME_TEST(d.lock() == a);
    PRIME_TEST(!b.expired());
    PRIME_TEST(!c.expired());
    PRIME_TEST(!d.expired());

    a.reset();

    PRIME_TEST(b.expired());
    PRIME_TEST(c.expired());
    PRIME_TEST(d.expired());
    PRIME_TEST(b.lock().get() == NULL);
    PRIME_TEST(c.lock().get() == NULL);
    PRIME_TEST(d.lock().get() == NULL);

    b.reset();
    c.reset();
    d.reset();
}
}

#endif
