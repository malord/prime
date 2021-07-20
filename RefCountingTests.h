// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_REFCOUNTINGTESTS_H
#define PRIME_REFCOUNTINGTESTS_H

#include "Config.h"

namespace Prime {

namespace RefCountingTestsPrivate {

#ifndef PRIME_TEST_TRACE
#define PRIME_TEST_TRACE(...) ((void)0)
#endif

    class Thing {
    public:
        static int alive;

        Thing()
            : _counter(1)
        {
            PRIME_TEST_TRACE("ctor");
            ++alive;
        }

        virtual ~Thing()
        {
            PRIME_TEST_TRACE("dtor");
            --alive;
        }

        virtual void retain() const
        {
            PRIME_TEST_TRACE("retain");
            _counter.increment();
        }

        virtual int release() const
        {
            PRIME_TEST_TRACE("release");
            int count = _counter.decrement();
            PRIME_ASSERT(count >= 0);
            if (!count) {
                delete this;
            }
            return count;
        }

        virtual int getRefCount() const
        {
            return _counter.get();
        }

    private:
        mutable NonAtomicCounter _counter;
    };

    inline void RefPtrRetain(Thing* rc)
    {
        rc->retain();
    }

    inline void RefPtrRelease(Thing* rc)
    {
        rc->release();
    }

    int Thing::alive = 0;

    static RefPtr<Thing> CreatePass()
    {
        return PassRef(new Thing);
    }

    static RefPtr<Thing> CreateRef()
    {
        return PassRef(new Thing);
    }

    static void TestCreatePass()
    {
        PRIME_TEST_TRACE("-- TestCreatePass");
        PRIME_ASSERT(Thing::alive == 0);
        {
            RefPtr<Thing> r1 = CreatePass();
            PRIME_ASSERT(Thing::alive > 0);
        }

        PRIME_ASSERT(Thing::alive == 0);
    }

    static void TestCreateRef()
    {
        PRIME_TEST_TRACE("-- TestCreateRef");
        PRIME_ASSERT(Thing::alive == 0);
        {
            RefPtr<Thing> r1 = CreateRef();
            PRIME_ASSERT(Thing::alive > 0);
        }

        PRIME_ASSERT(Thing::alive == 0);
    }

    static void TestLossOfType()
    {
        PRIME_TEST_TRACE("-- TestLossOfType");
        // If a RefPtr is somehow cast to a RefPtr, make sure things work.
        PRIME_ASSERT(Thing::alive == 0);
        {
            RefPtr<Thing> r1 = PassRef(new Thing);
            PRIME_ASSERT(Thing::alive > 0);
            RefPtr<Thing> r2 = (RefPtr<Thing>&)r1;
            PRIME_ASSERT(Thing::alive > 0);
        }

        PRIME_ASSERT(Thing::alive == 0);
    }
}

inline void RefCountingTests()
{
    using namespace RefCountingTestsPrivate;

    TestCreatePass();
    TestCreateRef();
    TestLossOfType();
}
}

#endif
