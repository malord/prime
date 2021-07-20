// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STRINGTESTS_H
#define PRIME_STRINGTESTS_H

#include "Convert.h"
#include "StringUtils.h"

namespace Prime {

namespace StringTestsPrivate {

    inline void StringUtilsTests()
    {
        PRIME_TEST("Hello, world!" == StringViewTrim(StringView("\xc2\xa0\xc2\xa0\xe2\x80\xa8   \t\xe2\x80\xa9Hello, world! \t\xc2\xa0\xe2\x80\xa8\t\xe2\x80\xa9")));
        PRIME_TEST("Hello, world!" == StringViewTrim(StringView("\xc2\xa0\xc2\xa0\xe2\x80\xa8   \t\xe2\x80\xa9\t Hello, world!\xc2\xa0\xe2\x80\xa8\t\xe2\x80\xa9")));
        PRIME_TEST("Hello, world!" == StringViewTrim(StringView("\t  \t Hello, world! \t\t  \t")));
        PRIME_TEST("Hello, world!" == StringViewTrim(StringView("\t  \t Hello, world! \t\t  \t"), asciiWhitespaceChars));
        PRIME_TEST("Hello, world!" == StringViewTrim(StringView("Hello, world!")));
        PRIME_TEST("Hello, world!" == StringViewTrim(StringView("Hello, world!"), asciiWhitespaceChars));
        PRIME_TEST("" == StringViewTrim(StringView(""), asciiWhitespaceChars));
        PRIME_TEST("" == StringViewTrim(StringView("\xc2\xa0\xc2\xa0\xe2\x80\xa8   \t\xe2\x80\xa9 \t\xc2\xa0\xe2\x80\xa8\t\xe2\x80\xa9")));
        PRIME_TEST("" == StringViewTrim(StringView("\xc2\xa0\xc2\xa0\xe2\x80\xa8   \t\xe2\x80\xa9\t \xc2\xa0\xe2\x80\xa8\t\xe2\x80\xa9")));
        PRIME_TEST("" == StringViewTrim(StringView("\t  \t  \t\t  \t")));
        PRIME_TEST("" == StringViewTrim(StringView("\t  \t  \t\t  \t"), asciiWhitespaceChars));
        PRIME_ASSERT(StringsEqual("world!", StringLastComponent("Hello,\t  \tworld!", asciiWhitespaceChars)));
        PRIME_ASSERT(StringsEqual("world!", StringLastComponent("Hello,\xc2\xa0\xc2\xa0\xe2\x80\xa8   \t\xe2\x80\xa9world!", utf8WhitespaceChars)));
        PRIME_ASSERT(StringsEqual("", StringLastComponent("\xc2\xa0\xc2\xa0\xe2\x80\xa8   \t\xe2\x80\xa9", utf8WhitespaceChars)));
        PRIME_ASSERT(StringsEqual("", StringLastComponent("", utf8WhitespaceChars)));
        PRIME_ASSERT(StringsEqual("world", StringLastComponent("\xc2\xa0\xc2\xa0\xe2\x80\xa8   \t\xe2\x80\xa9world", utf8WhitespaceChars)));

        {
            std::vector<std::string> pieces = StringSplitOnSeparators("\xc2\xa0\xc2\xa0Well \xe2\x80\xa8   \t\xe2\x80\xa9\t Hello, world!\xc2\xa0\xe2\x80\xa8\tCooey!\xe2\x80\xa9", utf8WhitespaceChars, SplitSkipEmpty);
            PRIME_TEST(pieces.size() == 4 && pieces[0] == "Well" && pieces[1] == "Hello," && pieces[2] == "world!" && pieces[3] == "Cooey!");
        }

        PRIME_TEST(StringsEqualIgnoringNonAlphanumeric("mr. lord", "Mr Lord"));
        PRIME_TEST(!StringsEqualIgnoringNonAlphanumeric("mr. lord", "MrLord"));
        PRIME_TEST(!StringsEqualIgnoringNonAlphanumeric("ms. lord", "Mr Lord"));
        PRIME_TEST(StringsEqualIgnoringNonAlphanumeric("123 456 789", "12 34 56 78 9"));
        PRIME_TEST(StringsEqualIgnoringNonAlphanumeric("123 456 789", "123456789"));
        PRIME_TEST(!StringsEqualIgnoringNonAlphanumeric("123 456a 789", "123456a789"));
        PRIME_TEST(!StringsEqualIgnoringNonAlphanumeric("123 456a 789", "123 456a7 89"));
        PRIME_TEST(StringsEqualIgnoringNonAlphanumeric("123 456a 789", "123 456a 78 9"));
        PRIME_TEST(StringsEqualIgnoringNonAlphanumeric("123-456a-789", "123-456a-78.9"));

        {
            HybridWordParser wp("mr.\xc2\x85\xc2\xa0Lord");
            PRIME_TEST(wp.next().value_or("") == "mr");
            PRIME_TEST(wp.next().value_or("") == "Lord");
            PRIME_TEST(!wp.next());
        }
        {
            HybridWordParser wp(".\xc2\x85mr.\xc2\x85\xc2\xa0Lord\xc2\x85");
            PRIME_TEST(wp.next().value_or("") == "mr");
            PRIME_TEST(wp.next().value_or("") == "Lord");
            PRIME_TEST(!wp.next());
        }
        PRIME_TEST(StringsEqualIgnoringNonAlphanumeric("mr.\xc2\x85\xc2\xa0Lord", "mr lord"));

        PRIME_TEST(StringToInitials("Elliot John Lord") == "EJL");

        PRIME_TEST(ASCIIOnlyAlphanumericUppercase("ng13 8DY") == "NG138DY");
        PRIME_TEST(ASCIIOnlyAlphanumeric("ng13 8DY") == "ng138DY");

        PRIME_TEST(StringExtractNumber("£123.45") == "123.45");
        PRIME_TEST(StringExtractNumber("pays £123.45 monthly") == "123.45");
        PRIME_TEST(StringExtractNumber("pays £123 monthly") == "123");
        PRIME_TEST(StringExtractNumber("pays £123. monthly") == "123.");
        PRIME_TEST(StringExtractNumber("pays £.123 monthly") == ".123");
        PRIME_TEST(StringExtractNumber("pays £-.123 monthly") == "-.123");
        PRIME_TEST(StringExtractNumber("123.45") == "123.45");
        PRIME_TEST(StringExtractNumber("-123.45") == "-123.45");
        PRIME_TEST(StringExtractNumber("-.45") == "-.45");
        PRIME_TEST(StringExtractNumber(".45") == ".45");
    }
}

inline void StringTests()
{
    using namespace StringTestsPrivate;

    StringUtilsTests();
}
}

#endif
