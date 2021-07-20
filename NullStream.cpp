// Copyright 2000-2021 Mark H. P. Lord

#include "NullStream.h"

namespace Prime {

ptrdiff_t NullStream::readSome(void*, size_t, Log*)
{
    return 0;
}

ptrdiff_t NullStream::writeSome(const void*, size_t maxBytes, Log*)
{
    return maxBytes;
}

}
