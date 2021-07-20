// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_REGEXTESTS_H
#define PRIME_REGEXTESTS_H

#include "DateTime.h"
#include "Regex.h"

namespace Prime {

#ifdef PRIME_HAVE_REGEX

namespace RegexTestsPrivate {

    inline void RegexDateTest()
    {
        StringView str("07-Dec-20");

        Regex dateRegex("^(\\d+)-(\\w+)-(\\d+)$");
        Regex::Match dateMatch;
        PRIME_TEST(dateRegex.search(dateMatch, str));
        std::string group1 = dateMatch.getGroup(str, 0);
        std::string group2 = dateMatch.getGroup(str, 1);
        std::string group3 = dateMatch.getGroup(str, 2);
        std::string group4 = dateMatch.getGroup(str, 3);
        PRIME_TEST(group1 == str);
        PRIME_TEST(group2 == "07");
        PRIME_TEST(group3 == "Dec");
        PRIME_TEST(group4 == "20");

        Date date(
            ToInt(dateMatch.getGroup(str, 3), -1, 10),
            Date::parseRFC1123MonthName(dateMatch.getGroup(str, 2)),
            ToInt(dateMatch.getGroup(str, 1), -1, 10));
        PRIME_TEST(date.getDay() == 7);
        PRIME_TEST(date.getMonth() == 12);
        PRIME_TEST(date.getYear() == 20);
    }

}

#endif // PRIME_HAVE_REGEX

inline void RegexTests()
{
#ifdef PRIME_HAVE_REGEX
    using namespace RegexTestsPrivate;

    RegexDateTest();
#endif
}
}

#endif
