// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OSX_VALUENSOBJECT_H
#define PRIME_OSX_VALUENSOBJECT_H

#include "../Value.h"

namespace Prime {

PRIME_PUBLIC bool UnsafeConvert(Value& output, id object);

/// Returns an autoreleased object.
PRIME_PUBLIC id ToNSObject(const Value& value);
}

#endif
