// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UID_H
#define PRIME_UID_H

#include "Common.h"

namespace Prime {

// Each UID is encoded as four 32-bit unsigned integers, e.g., aaaaaaaa-bbbb-bbbb-cccc-ccccdddddddd becomes
// 0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc, 0xdddddddd, and can be generated on Mac/Windows/Linux using this Python
// script (Windows needs uuidgen to be in the PATH):
//
//     import os, sys
//     with os.popen('uuidgen', 'r') as p:
//         u = p.readline().lower()
//         str = "0x%s, 0x%s, 0x%s, 0x%s" % (u[0:8], u[9:13] + u[14:18], u[19:23] + u[24:28], u[28:36])
//         sys.stdout.write(str)
//
/// A unique identifier.
class UID {
public:
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;

    UID()
    PRIME_NOEXCEPT { }

    /// aaaaaaaa-bbbb-bbbb-cccc-ccccdddddddd
    UID(uint32_t initA, uint32_t initB, uint32_t initC, uint32_t initD)
    PRIME_NOEXCEPT : a(initA), b(initB), c(initC), d(initD)
    {
    }

    /// aaaaaaaa-bbbb-cccc-dddd-eeeeffffffff
    UID(uint32_t initA, uint16_t initB, uint16_t initC, uint16_t initD, uint16_t initE, uint32_t initF)
    PRIME_NOEXCEPT : a(initA),
                     b((uint32_t)(initB << 16) | initC),
                     c((uint32_t)(initD << 16) | initE),
                     d(initF)
    {
    }

    // TODO: parsing strings

    bool operator==(const UID& rhs) const PRIME_NOEXCEPT { return a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d; }

    bool operator<(const UID& rhs) const PRIME_NOEXCEPT
    {
        if (a < rhs.a)
            return true;
        if (a > rhs.a)
            return false;

        if (b < rhs.b)
            return true;
        if (b > rhs.b)
            return false;

        if (c < rhs.c)
            return true;
        if (c > rhs.c)
            return false;

        if (d < rhs.d)
            return true;
        return false;
    }

    PRIME_IMPLIED_COMPARISONS_OPERATORS(const UID&)
};

}

#endif
