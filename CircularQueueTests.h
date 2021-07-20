// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CIRCULARQUEUETESTS_H
#define PRIME_CIRCULARQUEUETESTS_H

#include "CircularQueue.h"
#include "NumberUtils.h"

namespace Prime {

namespace CircularQueueTestsPrivate {

    static void MovingAverageTest()
    {
        MovingAverage<float> ma;
        ma.init(4);
        ma.clear();

        ma.write(1.0f);
        PRIME_TEST(AlmostEqual(ma.get(), 1.0f, 0.001f));

        ma.write(3.0f);
        PRIME_TEST(AlmostEqual(ma.get(), 2.0f, 0.001f));

        ma.write(7.0f);
        PRIME_TEST(AlmostEqual(ma.get(), (1.0f + 3.0f + 7.0f) / 3.0f, 0.001f));

        ma.write(9.0f);
        PRIME_TEST(AlmostEqual(ma.get(), (1.0f + 3.0f + 7.0f + 9.0f) / 4.0f, 0.001f));

        ma.write(8.0f);
        PRIME_TEST(AlmostEqual(ma.get(), (8.0f + 3.0f + 7.0f + 9.0f) / 4.0f, 0.001f));

        ma.write(2.0f);
        PRIME_TEST(AlmostEqual(ma.get(), (8.0f + 2.0f + 7.0f + 9.0f) / 4.0f, 0.001f));

        ma.write(10.0f);
        PRIME_TEST(AlmostEqual(ma.get(), (8.0f + 2.0f + 10.0f + 9.0f) / 4.0f, 0.001f));

        ma.write(14.0f);
        PRIME_TEST(AlmostEqual(ma.get(), (8.0f + 2.0f + 10.0f + 14.0f) / 4.0f, 0.001f));

        ma.write(47.0f);
        PRIME_TEST(AlmostEqual(ma.get(), (47.0f + 2.0f + 10.0f + 14.0f) / 4.0f, 0.001f));
    }

    static void CircularQueueTest()
    {
        CircularQueue<float>::Buffer<9> q;

        for (int i = 0; i != 9; ++i) {
            if (!q.push_back((float)i)) {
                RuntimeError("TestCircularQueue: failed to write element.");
            }

            if (q.size() != (size_t)(i + 1)) {
                RuntimeError("TestCircularQueue: size() is wrong after push_back().");
            }
        }

        if (q.push_back(11)) {
            RuntimeError("TestCircularQueue: wrote past end.");
        }

        for (int i = 0; i != 9; ++i) {
            if (q.pop_front() != i) {
                RuntimeError("TestCircularQueue: read wrong element.");
            }

            if (q.size() != (size_t)(8 - i)) {
                RuntimeError("TestCircularQueue: size() is wrong after pop_front().");
            }
        }

        for (int i = 0; i != 9; ++i) {
            q.push_back((float)i);
        }

        q.pop_front();

        q.push_back(47);

        if (q.size() != 9) {
            RuntimeError("TestCircularQueue: size wrong after wrap around.");
        }

        for (int i = 1; i != 9; ++i) {
            if (q.pop_front() != i) {
                RuntimeError("TestCircularQueue: read wrong element.");
            }
        }

        if (q.pop_front() != 47) {
            RuntimeError("TestCircularQueue: read wrong element.");
        }

        if (q.size() != 0 || !q.empty()) {
            RuntimeError("TestCircularQueue: not empty!");
        }
    }

    static void CircularQueueTest2()
    {
        CircularQueue<int>::Buffer<5> q;

        q.push_back(1);
        PRIME_TEST(q.size() == 1);
        q.push_back(2);
        PRIME_TEST(q.size() == 2);
        q.push_back(3);
        PRIME_TEST(q.size() == 3);
        q.push_back(4);
        PRIME_TEST(q.size() == 4);
        q.push_back(5);
        PRIME_TEST(q.size() == 5);
        PRIME_TEST(q.full());
        PRIME_TEST(q.pop_front() == 1);
        PRIME_TEST(q.size() == 4);
        PRIME_TEST(q.pop_front() == 2);
        PRIME_TEST(q.size() == 3);
        PRIME_TEST(q.pop_front() == 3);
        PRIME_TEST(q.size() == 2);
        PRIME_TEST(q.pop_front() == 4);
        PRIME_TEST(q.size() == 1);
        q.push_back(6);
        PRIME_TEST(q.size() == 2);
        q.push_back(7);
        PRIME_TEST(q.size() == 3);
        q.push_back(8);
        PRIME_TEST(q.size() == 4);
        q.push_back(9);
        PRIME_TEST(q.size() == 5);
        PRIME_TEST(q.full());
        PRIME_TEST(!q.push_back(10));
        PRIME_TEST(q.pop_front() == 5);
        PRIME_TEST(q.size() == 4);
        PRIME_TEST(q.pop_front() == 6);
        PRIME_TEST(q.size() == 3);
        PRIME_TEST(q.pop_front() == 7);
        PRIME_TEST(q.size() == 2);
        PRIME_TEST(q.pop_front() == 8);
        PRIME_TEST(q.size() == 1);
        PRIME_TEST(q.pop_front() == 9);
        PRIME_TEST(q.size() == 0);
        PRIME_TEST(q.empty());
        q.push_back(1);
        q.push_back(2);
        q.push_back(3);
        q.push_back(4);
        q.push_back(5);
        q.remove(4);
        q.push_back(6);
        PRIME_TEST(q.size() == 5);
        PRIME_TEST(q.pop_front() == 1);
        PRIME_TEST(q.pop_front() == 2);
        PRIME_TEST(q.pop_front() == 3);
        PRIME_TEST(q.pop_front() == 4);
        PRIME_TEST(q.pop_front() == 6);
        PRIME_TEST(q.empty());
        q.push_back(1);
        q.remove(0);
        PRIME_TEST(q.empty());
        PRIME_TEST(q.size() == 0);
    }
}

inline void CircularQueueTests()
{
    using namespace CircularQueueTestsPrivate;

    CircularQueueTest();
    CircularQueueTest2();
    MovingAverageTest();
}

}

#endif
