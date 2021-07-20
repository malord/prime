// Copyright 2000-2021 Mark H. P. Lord

#include "SystemMemoryManager.h"
#include <stdlib.h>
#include <string.h>

namespace Prime {

SystemMemoryManager::SystemMemoryManager()
{
}

void* SystemMemoryManager::allocate(size_t size)
{
    return systemAllocate(size);
}

void SystemMemoryManager::free(void* ptr)
{
    systemFree(ptr);
}

void* SystemMemoryManager::reallocate(void* ptr, size_t newSize)
{
    return systemReallocate(ptr, newSize);
}

void* SystemMemoryManager::allocateAligned(size_t size, size_t alignment)
{
    // Alignment must be a power of two.
    PRIME_ASSERT((alignment & (alignment - 1)) == 0);

#if PRIME_MSC_AND_NEWER(1300)
    for (;;) {
        void* ptr = _aligned_malloc(size, alignment);

        if (ptr || !outOfMemory()) {
            return ptr;
        }
    }
#else
    return MemoryManager::allocateAligned(size, alignment);
#endif
}

void SystemMemoryManager::freeAligned(void* ptr)
{
#if PRIME_MSC_AND_NEWER(1300)
    if (ptr) {
        _aligned_free(ptr);
    }
#else
    MemoryManager::freeAligned(ptr);
#endif
}

void* SystemMemoryManager::reallocateAligned(void* ptr, size_t newSize, size_t newAlignment)
{
#if PRIME_MSC_AND_NEWER(1300)
    for (;;) {
        void* newPtr = _aligned_realloc(ptr, newSize, newAlignment);

        if (newPtr || !outOfMemory()) {
            return newPtr;
        }
    }
#else
    return MemoryManager::reallocateAligned(ptr, newSize, newAlignment);
#endif
}

void* SystemMemoryManager::systemAllocate(size_t size)
{
    for (;;) {
        void* ptr = ::malloc(size);

        if (ptr || !getGlobal()->outOfMemory()) {
            return ptr;
        }
    }
}

void SystemMemoryManager::systemFree(void* ptr)
{
    ::free(ptr);
}

void* SystemMemoryManager::systemReallocate(void* ptr, size_t newSize)
{
    for (;;) {
        void* newPtr = ::realloc(ptr, newSize);

        if (newPtr || !getGlobal()->outOfMemory()) {
            return newPtr;
        }
    }
}

}
