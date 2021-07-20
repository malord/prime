// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UTF8REWINDSUPPORT_H
#define PRIME_UTF8REWINDSUPPORT_H

#include "Config.h"

#ifndef PRIME_NO_UTF8REWIND

namespace Prime {

/// Registers all the global hooks necessary to integrate utf8rewind in to Prime (e.g., StringToLower(),
/// StringsEqualIgnoringCase(), StringCompareIngoringCase()).
class PRIME_PUBLIC UTF8RewindSupport {
public:
    explicit UTF8RewindSupport();

    ~UTF8RewindSupport();

    static void init();

    static void close();
};
}

#endif // PRIME_NO_UTF8REWIND

#endif
