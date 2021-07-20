// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SYSTEMMEMORYMANAGER_H
#define PRIME_SYSTEMMEMORYMANAGER_H

#include "MemoryManager.h"

namespace Prime {

/// MemoryManager implementation that uses C++ library memory allocation functions.
class PRIME_PUBLIC SystemMemoryManager : public MemoryManager {
public:
    SystemMemoryManager();

    // MemoryManager implementation.
    virtual void* allocate(size_t size) PRIME_OVERRIDE;
    virtual void free(void* ptr) PRIME_OVERRIDE;
    virtual void* reallocate(void* ptr, size_t newSize) PRIME_OVERRIDE;
    virtual void* allocateAligned(size_t size, size_t alignment) PRIME_OVERRIDE;
    virtual void freeAligned(void* ptr) PRIME_OVERRIDE;
    virtual void* reallocateAligned(void* ptr, size_t newSize, size_t newAlignment) PRIME_OVERRIDE;

protected:
    static void* systemAllocate(size_t size);
    static void systemFree(void* ptr);
    static void* systemReallocate(void* ptr, size_t newSize);
};
}

#endif
