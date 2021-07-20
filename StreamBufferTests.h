// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STREAMBUFFERTESTS_H
#define PRIME_STREAMBUFFERTESTS_H

#include "StreamBuffer.h"

namespace Prime {

inline void StreamBufferTests(Log* log)
{
    static const char text[] = "\n"
                               "Line 1\r\n"
                               "Line 2\n"
                               "Line 3\r"
                               "Line 4\r\n"
                               "Line 123\r\n"
                               "Line 1234\n"
                               "Line 12345\n"
                               "The end";

    StreamBuffer sb(text, sizeof(text) - 1);

    for (int number = 0;; ++number) {
        char buffer[9];
        char* newlinePointer;
        if (!sb.readLine(buffer, sizeof(buffer), Log::getGlobal(), &newlinePointer)) {
            return;
        }

        if (!buffer[0]) {
            break;
        }

        *newlinePointer = 0;
        log->trace("%d: %s", number, buffer);
    }
}
}

#endif
