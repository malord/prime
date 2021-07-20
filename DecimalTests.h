// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DECIMALTESTS_H
#define PRIME_DECIMALTESTS_H

#include "Decimal.h"

namespace Prime {

namespace DecimalTestsPrivate {

    inline void Test1()
    {
        // Test string parsing and rounding.
        {
            Decimal a = Decimal::parseOrZero("1.2345").getRounded(3, Decimal::RoundModeHalfAwayFromZero);
            PRIME_TEST(a.toString() == "1.235");
        }
        {
            Decimal a = Decimal::parseOrZero("1.2345").getRounded(3, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "1.234");
        }
        {
            Decimal a = Decimal::parseOrZero("1.2356").getRounded(3, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "1.236");
        }
        {
            Decimal a = Decimal::parseOrZero("-1.2345").getRounded(3, Decimal::RoundModeHalfAwayFromZero);
            PRIME_TEST(a.toString() == "-1.235");
        }
        {
            Decimal a = Decimal::parseOrZero("-1.2345").getRounded(3, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "-1.234");
        }
        {
            Decimal a = Decimal::parseOrZero("-1.2356").getRounded(3, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "-1.236");
        }
    }

    inline void Test2()
    {
        // Test number to string rounding.
        {
            Decimal a = Decimal::parseOrZero("1.2345");
            PRIME_TEST(a.toString(3, Decimal::RoundModeHalfAwayFromZero) == "1.235");
        }
        {
            Decimal a = Decimal::parseOrZero("1.2345");
            PRIME_TEST(a.toString(3, Decimal::RoundModeHalfToEven) == "1.234");
        }
        {
            Decimal a = Decimal::parseOrZero("1.2356");
            PRIME_TEST(a.toString(3, Decimal::RoundModeHalfToEven) == "1.236");
        }
        {
            Decimal a = Decimal::parseOrZero("-1.2345");
            PRIME_TEST(a.toString(3, Decimal::RoundModeHalfAwayFromZero) == "-1.235");
        }
        {
            Decimal a = Decimal::parseOrZero("-1.2345");
            PRIME_TEST(a.toString(3, Decimal::RoundModeHalfToEven) == "-1.234");
        }
        {
            Decimal a = Decimal::parseOrZero("-1.2356");
            PRIME_TEST(a.toString(3, Decimal::RoundModeHalfToEven) == "-1.236");
        }
    }

    inline void Test3()
    {
        // Test rounding.
        {
            Decimal a = Decimal::parseOrZero("1.2345");
            a = a.getRounded(3, Decimal::RoundModeHalfAwayFromZero);
            PRIME_TEST(a.toString() == "1.235");
        }
        {
            Decimal a = Decimal::parseOrZero("1.2345");
            a = a.getRounded(3, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "1.234");
        }
        {
            Decimal a = Decimal::parseOrZero("1.2356");
            a = a.getRounded(3, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "1.236");
        }
        {
            Decimal a = Decimal::parseOrZero("-1.2345");
            a = a.getRounded(3, Decimal::RoundModeHalfAwayFromZero);
            PRIME_TEST(a.toString() == "-1.235");
        }
        {
            Decimal a = Decimal::parseOrZero("-1.2345");
            a = a.getRounded(3, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "-1.234");
        }
        {
            Decimal a = Decimal::parseOrZero("-1.2356");
            a = a.getRounded(3, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "-1.236");
        }
        {
            Decimal a = Decimal::parseOrZero("23.5");
            a = a.getRounded(0, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "24");
        }
        {
            Decimal a = Decimal::parseOrZero("24.5");
            a = a.getRounded(0, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "24");
        }
        {
            Decimal a = Decimal::parseOrZero("-23.5");
            a = a.getRounded(0, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "-24");
        }
        {
            Decimal a = Decimal::parseOrZero("-24.5");
            a = a.getRounded(0, Decimal::RoundModeHalfToEven);
            PRIME_TEST(a.toString() == "-24");
        }
    }
}

inline void DecimalTests()
{
    using namespace DecimalTestsPrivate;

    Test1();
    Test2();
    Test3();

    PRIME_TEST(Decimal(123).toStringWithThousandSeparator(',', 0) == "123");
    PRIME_TEST(Decimal(1234).toStringWithThousandSeparator(',', 0) == "1,234");
    PRIME_TEST(Decimal(1234.25).toStringWithThousandSeparator(',', 2) == "1,234.25");
    PRIME_TEST(Decimal(123.25).toStringWithThousandSeparator(',', 2) == "123.25");
    PRIME_TEST(Decimal(12.25).toStringWithThousandSeparator(',', 2) == "12.25");
    PRIME_TEST(Decimal(1.25).toStringWithThousandSeparator(',', 2) == "1.25");
    PRIME_TEST(Decimal(0.25).toStringWithThousandSeparator(',', 2) == "0.25");
    PRIME_TEST(Decimal(0).toStringWithThousandSeparator(',', 0) == "0");
    PRIME_TEST(Decimal(-0.25).toStringWithThousandSeparator(',', 2) == "-0.25");
    PRIME_TEST(Decimal(-1.25).toStringWithThousandSeparator(',', 2) == "-1.25");
    PRIME_TEST(Decimal(-12.25).toStringWithThousandSeparator(',', 2) == "-12.25");
    PRIME_TEST(Decimal(-123.25).toStringWithThousandSeparator(',', 2) == "-123.25");
    PRIME_TEST(Decimal(-1234.25).toStringWithThousandSeparator(',', 2) == "-1,234.25");
    PRIME_TEST(Decimal(-123456789.25).toStringWithThousandSeparator(',', 2) == "-123,456,789.25");
    PRIME_TEST(Decimal(123456789.25).toStringWithThousandSeparator(',', 2) == "123,456,789.25");
    PRIME_TEST(Decimal(123456789.25).toStringWithThousandSeparator(',', 0) == "123,456,789");
    PRIME_TEST(Decimal(123456789.254).toStringWithThousandSeparator(',', 2) == "123,456,789.25");
    PRIME_TEST(Decimal(123456789.255).toStringWithThousandSeparator(',', 2) == "123,456,789.26");
    PRIME_TEST(Decimal(-123456789.25).toStringWithThousandSeparator(',', 0) == "-123,456,789");
    PRIME_TEST(Decimal(-123456789.254).toStringWithThousandSeparator(',', 2) == "-123,456,789.25");
    PRIME_TEST(Decimal(-123456789.255).toStringWithThousandSeparator(',', 2) == "-123,456,789.26");
    PRIME_TEST(Decimal(-123456789.25591).toStringWithThousandSeparator(',') == "-123,456,789.25591");
    PRIME_TEST(Decimal(-123456789.2).toStringWithThousandSeparator(',', 2) == "-123,456,789.20");

    PRIME_TEST(Decimal::parseOrZero("1234.5").getRounded(2, Decimal::RoundModeTowardsZero).toString() == "1234.5");
    PRIME_TEST(Decimal::parseOrZero("1234.55").getRounded(2, Decimal::RoundModeTowardsZero).toString() == "1234.55");
    PRIME_TEST(Decimal::parseOrZero("1234.555").getRounded(2, Decimal::RoundModeTowardsZero).toString() == "1234.55");
}
}

#endif
