// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TESTS_H
#define PRIME_TESTS_H

#include "Config.h"

#ifndef PRIME_FINAL

#include "CSVTests.h"
#include "CircularQueueTests.h"
#include "DateTimeTests.h"
#include "DecimalTests.h"
#include "DoubleLinkListTests.h"
#include "OpenSSLAESTests.h"
#include "PathTests.h"
#include "RefCountingTests.h"
#include "SharedPtrTests.h"
#include "StreamBufferTests.h"
#include "StringStreamTests.h"
#include "StringTests.h"
#include "TextEncodingTests.h"
#include "XMLTests.h"
#include "RegexTests.h"

namespace Prime {

inline void AllPrimeTests(Log* log)
{
    RegexTests();
    StringTests();
    DecimalTests();
    PathTests();
    CSVTests();
    CircularQueueTests();
    TextEncodingTests();
    SharedPtrTests();
    StringStreamTests();
    RefCountingTests();
    XMLTests(log);
    StreamBufferTests(log);
    DoubleLinkListTests();
    DateTimeTests();
    OpenSSLAESTests();
}
}

#endif

#endif
