// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MEMORYMANAGER_H
#define PRIME_MEMORYMANAGER_H

#include "RefCounting.h"
#include "UIDCast.h"

namespace Prime {

/// An object capable of allocating, deallocating and reallocating memory.
class PRIME_PUBLIC MemoryManager : public RefCounted {
    PRIME_DECLARE_UID_CAST_BASE(0xded29223, 0x882745e2, 0x8609f30a, 0x8ccc6190)

public:
    /// If not set, a standard memory manager for the platform is used. Use the functions in Common.h (Allocate,
    /// Free, AllocateAligned, etc.) instead of using this directly.
    static MemoryManager* getGlobal() { return _global ? _global : ((void)setGlobal(NULL), _global); }

    /// Use setGlobal(NULL) to reset a standard memory manager.
    static void setGlobal(MemoryManager* memoryManager);

    /// The long-term memory manager is to be used for long-term allocations (e.g., the level in a game). These
    /// allocations can be held in a different heap to reduce fragmentation.
    static MemoryManager* getGlobalLongTerm() { return _globalLongTerm ? _globalLongTerm : getGlobal(); }

    static void setGlobalLongTerm(MemoryManager* memoryManager) { _globalLongTerm = memoryManager; }

    /// If this MemoryManager is assigned as one of the globals, nulls the global.
    virtual ~MemoryManager();

    /// Returns true if the memory allocation should be retried.
    virtual bool outOfMemory();

    virtual void* allocate(size_t size) = 0;

    virtual void free(void* ptr) = 0;

    virtual void* reallocate(void* ptr, size_t newSize) = 0;

    void* allocateZeroed(size_t size);

    // Aligned allocations are emulated by MemoryManager for implementations that don't override them, but it's
    // extremely wasteful for large alignments.

    virtual void* allocateAligned(size_t size, size_t alignment);

    virtual void freeAligned(void* ptr);

    virtual void* reallocateAligned(void* ptr, size_t newSize, size_t newAlignment);

    void* allocateAlignedZeroed(size_t size, size_t alignment);

private:
    static MemoryManager* _global;
    static MemoryManager* _globalLongTerm;
};
}

inline void* operator new(size_t size, Prime::MemoryManager* memoryManager)
{
    return memoryManager->allocate(size);
}

inline void operator delete(void* ptr, Prime::MemoryManager* memoryManager)
{
    memoryManager->free(ptr);
}

inline void* operator new[](size_t size, Prime::MemoryManager* memoryManager)
{
    return memoryManager->allocate(size);
}

inline void operator delete[](void* ptr, Prime::MemoryManager* memoryManager)
{
    memoryManager->free(ptr);
}

#endif
