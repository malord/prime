// Copyright 2000-2021 Mark H. P. Lord

#include "RefCounting.h"

namespace Prime {

//
// RefCounted
//

void RefCounted::released() PRIME_NOEXCEPT
{
    delete this;
}
}
