// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OSX_SCOPEDAUTORELEASEPOOL_H
#define PRIME_OSX_SCOPEDAUTORELEASEPOOL_H

#include "../Config.h"
#include <Foundation/Foundation.h>

#if !__has_feature(objc_arc)

namespace Prime {

/// Creates an NSAutoreleasePool which is destroyed when destructed.
class ScopedAutoreleasePool {
public:
    ScopedAutoreleasePool()
    {
        _pool = [[NSAutoreleasePool alloc] init];
    }

    ~ScopedAutoreleasePool()
    {
        if (_pool) {
            [_pool drain];
        }
    }

private:
    NSAutoreleasePool* _pool;
};

}
#endif // __has_feature(objc_arc)

#endif
