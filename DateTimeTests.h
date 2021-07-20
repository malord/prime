// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DATETIMETESTS_H
#define PRIME_DATETIMETESTS_H

#include "Clocks.h"
#include "Config.h"

namespace Prime {

namespace DateTimeTestsPrivate {

    inline int64_t DateToUnixDay1(int year, int month, int dayOfMonth)
    {
        if (month < 3) {
            month += 13;
            year -= 1;
        } else {
            month += 1;
        }

        return (int64_t)(dayOfMonth + floor(30.6001 * month) + 365.0 * year + floor(year / 4.0) - floor(year / 100.0) + floor(year / 400.0) - 719591.0);
    }

    inline int64_t DateToUnixDay2(int year, int month, int dayOfMonth)
    {
        int64_t a = (14 - month) / 12;
        int64_t y = year + 4800 - a;
        int64_t m = month + 12 * a - 3;

        return dayOfMonth + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - (32045 + 2440588);

        // Given a date on the Julian calendar, you'd do:
        //return dayOfMonth + (153*m + 2) / 5 + 365*y + y/4 - (32083 + 2440588);
    }

    inline DateTime FromUnixDay1(int64_t unixDay)
    {
        // This is a Julian Day calculation modified so that 0 is at the Unix epoch.

        double z = (double)(unixDay + 719469);
        double g = z - 0.25;
        double a = floor(g / 36524.25);
        double b = a - floor(a / 4.0);
        int year = (int)(floor((b + g) / 365.25));
        double c = b + z - floor(365.25 * (year));
        int month = (int)((5.0 * c + 456.0) / 153.0);
        int dayOfMonth = (int)(c - (int)(30.6001 * (month)-91.4) + 0);
        if (month > 12) {
            year += 1;
            month -= 12;
        }

        return DateTime(year, month, dayOfMonth, 0, 0, 0);
    }

    inline DateTime FromUnixDay2(int64_t unixDay)
    {
        const int64_t y = 4716;
        const int64_t j = 1401;
        const int64_t m = 3;
        const int64_t n = 12;
        const int64_t r = 4;
        const int64_t p = 1461;
        const int64_t v = 3;
        const int64_t u = 5;
        const int64_t s = 153;
        const int64_t w = 2;
        const int64_t B = 274277;
        const int64_t G = -38;

        int64_t julianDay = unixDay + 2440588;

        int64_t g = ((3 * (((4 * julianDay + B) / 146097)) / 4)) + G;
        int64_t J_ = julianDay + j + g;
        int64_t Y_ = ((r * J_ + v) / p);
        int64_t T_ = (((r * J_ + v) % p) / r);
        int64_t M_ = ((u * T_ + w) / s);
        int64_t D_ = (((u * T_ + w) % s) / u);
        int64_t dayOfMonth = D_ + 1;
        int64_t month = ((M_ + m - 1) % n) + 1;
        int64_t year = Y_ - y + (((n + m - 1 - month) / n));

        return DateTime((int)year, (int)month, (int)dayOfMonth);
    }

    inline void DateTimeTestToDateTime(int64_t unixDay, int year, int month, int dayOfMonth)
    {
        DateTime g = DateTime::fromUnixDay(unixDay);
        PRIME_TEST(g.getYear() == year && g.getMonth() == month && g.getDay() == dayOfMonth);
        PRIME_TEST(FromUnixDay1(unixDay) == g);
        PRIME_TEST(FromUnixDay2(unixDay) == g);

#ifdef _MSC_VER
        if (unixDay >= 0)
#endif

        //#ifdef PRIME_OS_UNIX
        {
            time_t unixTime = (time_t)(unixDay * (60 * 60 * 24));
            tm* t = gmtime(&unixTime);
            PRIME_TEST(t->tm_year + 1900 == year && t->tm_mon + 1 == month && t->tm_mday == dayOfMonth);
        }
        //#endif
    }

    inline void DateTimeTestToUnixDay(int year, int month, int dayOfMonth, int64_t unixDay)
    {
        int64_t computed = DateTime(year, month, dayOfMonth).toUnixDay();
        PRIME_TEST(computed == unixDay);
        int64_t computed2 = DateToUnixDay1(year, month, dayOfMonth);
        PRIME_TEST(computed == computed2);
        int64_t computed3 = DateToUnixDay2(year, month, dayOfMonth);
        PRIME_TEST(computed == computed3);

#ifdef _MSC_VER
        if (year > 1969) {
#else
        if (year > 1900) {
#endif

            tm t;
            memset(&t, 0, sizeof(t));
            t.tm_year = year - 1900;
            t.tm_mon = month - 1;
            t.tm_mday = dayOfMonth;
#ifdef _MSC_VER
            int64_t unixComputed = _mkgmtime(&t) / (60 * 60 * 24);
#else
            int64_t unixComputed = timegm(&t) / (60 * 60 * 24);
#endif
            PRIME_TEST(unixComputed == unixDay);
        }
    }

    inline void DateTimeTestToBothWays(int year, int month, int dayOfMonth, int64_t unixDay)
    {
        DateTimeTestToUnixDay(year, month, dayOfMonth, unixDay);
        DateTimeTestToDateTime(unixDay, year, month, dayOfMonth);
    }

    inline void NextWeekTests()
    {
        PRIME_TEST(Date(2019, 12, 31).getNextMonth() == Date(2020, 01, 31));
        PRIME_TEST(Date(2020, 01, 31).getNextMonth() == Date(2020, 02, 29));
        PRIME_TEST(Date(2020, 02, 29).getNextMonth() == Date(2020, 03, 29));
        PRIME_TEST(Date(2020, 02, 29).getNextMonth(31) == Date(2020, 03, 31));
        PRIME_TEST(Date(2020, 03, 31).getNextMonth(31) == Date(2020, 04, 30));
        PRIME_TEST(Date(2020, 04, 30).getNextMonth(31) == Date(2020, 05, 31));

        DateTime firstPayment(Date(2021, 3, 1), Time(22, 55, 6));
        DateTime nextPayment =
            DateTime(
                DateTime(firstPayment).getDate().getNextMonth(), Time(0, 0, 0));
        PRIME_TEST(nextPayment.getYear() == 2021);
        PRIME_TEST(nextPayment.getMonth() == 4);
        PRIME_TEST(nextPayment.getDay() == 1);
        PRIME_TEST(nextPayment.getHour() == 0);
        PRIME_TEST(nextPayment.getMinute() == 0);
        PRIME_TEST(nextPayment.getSecond() == 0);
        
        const auto preferredDay = firstPayment.getDay();
        
        DateTime nextNextPayment = nextPayment;
        nextNextPayment.setDate(nextNextPayment.getDate().getNextMonth(preferredDay));
        nextNextPayment.setDate(nextNextPayment.getDate().getAddDays(0));
        
        PRIME_TEST(nextNextPayment.getYear() == 2021);
        PRIME_TEST(nextNextPayment.getMonth() == 5);
        PRIME_TEST(nextNextPayment.getDay() == 1);
        PRIME_TEST(nextNextPayment.getHour() == 0);
        PRIME_TEST(nextNextPayment.getMinute() == 0);
        PRIME_TEST(nextNextPayment.getSecond() == 0);
    }
}

inline void DateTimeTests()
{
    using namespace DateTimeTestsPrivate;

    DateTimeTestToBothWays(2012, 01, 17, 15356);
    DateTimeTestToBothWays(1970, 1, 1, 0);
    DateTimeTestToBothWays(1972, 2, 29, 789);
    DateTimeTestToBothWays(1972, 3, 1, 790);
    DateTimeTestToBothWays(1969, 12, 31, -1);
    DateTimeTestToBothWays(-4000, 4, 29, -2180379);

    NextWeekTests();
}
}

#endif
