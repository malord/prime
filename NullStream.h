// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_NULLSTREAM_H
#define PRIME_NULLSTREAM_H

#include "Stream.h"

namespace Prime {

class PRIME_PUBLIC NullStream : public Stream {
public:
    virtual ptrdiff_t readSome(void*, size_t, Log*) PRIME_OVERRIDE;

    virtual ptrdiff_t writeSome(const void*, size_t maxBytes, Log*) PRIME_OVERRIDE;
};

}

#endif
