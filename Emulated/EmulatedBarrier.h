// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_EMULATED_EMULATEDBARRIER_H
#define PRIME_EMULATED_EMULATEDBARRIER_H

#include "../RefCounting.h"
#include "../Semaphore.h"

namespace Prime {

/// Emulate a barrier threading primitive with a semaphore and atomic counter.
class PRIME_PUBLIC EmulatedBarrier {
public:
    EmulatedBarrier()
        : _waiting(0)
    {
    }

    EmulatedBarrier(int count, Log* log, const char* debugName = NULL)
        : _waiting(0)
    {
        PRIME_EXPECT(init(count, log, debugName));
    }

    bool init(int count, Log* log, const char* debugName = NULL);

    void close();

    bool isInitialised() { return _semaphore.isInitialised(); }

    /// Blocks until it's been called count times (as passed to init).
    void wait();

private:
    AtomicCounter _waiting;
    int32_t _count;
    Semaphore _semaphore;
};
}

#endif
