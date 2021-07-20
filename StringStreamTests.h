// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STRINGSTREAMTESTS_H
#define PRIME_STRINGSTREAMTESTS_H

#include "StringStream.h"

namespace Prime {

inline void StringStreamTests()
{
    StringStream ss;
    ss.reserve(13);
    ss.writeExact("Hello, world!", 13, Log::getGlobal());
    ss.setOffset(7, Log::getGlobal());
    ss.writeExact("Earth", 5, Log::getGlobal());
    PRIME_TEST(StringsEqual(ss.c_str(), "Hello, Earth!"));
    ss.setOffset(13, Log::getGlobal());
    for (int i = 0; i != 128; ++i) {
        ss.writeExact(" EXTERMINATE", 12, Log::getGlobal());
    }
    PRIME_TEST(memcmp(ss.c_str(), "Hello, Earth!", 13) == 0);
    ss.setOffset(13, Log::getGlobal());
    for (int i = 0; i != 128; ++i) {
        char got[12];
        memset(got, 0, sizeof(got));
        ss.readExact(got, sizeof(got), Log::getGlobal());
        PRIME_TEST(memcmp(got, " EXTERMINATE", 12) == 0);
    }
    char ch;
    PRIME_TEST(ss.readSome(&ch, 1, Log::getGlobal()) == 0);
}

}

#endif
