// Copyright 2000-2021 Mark H. P. Lord

#include "MemoryManager.h"
#include "SystemMemoryManager.h"
#include <string.h>

namespace Prime {

PRIME_DEFINE_UID_CAST_BASE(MemoryManager)

MemoryManager* MemoryManager::_global = NULL;
MemoryManager* MemoryManager::_globalLongTerm = NULL;

void MemoryManager::setGlobal(MemoryManager* memoryManager)
{
    if (memoryManager) {
        _global = memoryManager;
    } else {
        static SystemMemoryManager systemMemoryManager;
        _global = &systemMemoryManager;
    }
}

MemoryManager::~MemoryManager()
{
    if (_global == this) {
        _global = NULL;
    }

    if (_globalLongTerm == this) {
        _globalLongTerm = NULL;
    }
}

bool MemoryManager::outOfMemory()
{
    RuntimeError("Out of memory.");
    return false;
}

void* MemoryManager::allocateZeroed(size_t size)
{
    void* ptr = allocate(size);
    if (ptr) {
        memset(ptr, 0, size);
    }

    return ptr;
}

void* MemoryManager::allocateAlignedZeroed(size_t size, size_t alignment)
{
    void* ptr = allocateAligned(size, alignment);
    if (ptr) {
        memset(ptr, 0, size);
    }

    return ptr;
}

// This implementation works by allocating much more memory than is required in order to guarantee there will
// be room at the start of the block to align the pointer and to store the necessary header. For large alignments,
// this is alarmingly wasteful.

static const size_t headerSignature = (size_t)UINT32_C(0xabcd4c4d);

static const int headerSizeOffset = -1;
static const int headerBlockPointerOffset = -2;
static const int headerSignatureOffset = -3;
static const int headerSize = 4;

void* MemoryManager::allocateAligned(size_t size, size_t alignment)
{
    const size_t sizeOfHeader = headerSize * sizeof(size_t);
    void* mem;
    size_t addr;
    size_t aligned;
    size_t* header;

    // Alignment must be a power of two.
    PRIME_ASSERT((alignment & (alignment - 1)) == 0);

    // Our header values must be aligned.
    if (alignment < sizeof(size_t)) {
        alignment = sizeof(size_t);
    }

    // If you need significant alignment, this is somewhat wasteful.
    mem = allocate(size + alignment + sizeOfHeader);
    if (!mem) {
        return NULL;
    }

    addr = (size_t)mem + sizeOfHeader;
    aligned = (addr + (alignment - 1)) & (~(alignment - 1));

    header = (size_t*)(void*)aligned;
    header[headerSizeOffset] = size;
    header[headerBlockPointerOffset] = (size_t)mem;
    header[headerSignatureOffset] = headerSignature;

    return (void*)aligned;
}

void MemoryManager::freeAligned(void* ptr)
{
    size_t* header = (size_t*)ptr;

    PRIME_ASSERT(header[headerSignatureOffset] == headerSignature);

    free((void*)header[headerBlockPointerOffset]);
}

void* MemoryManager::reallocateAligned(void* ptr, size_t newSize, size_t newAlignment)
{
    size_t* header = (size_t*)ptr;
    size_t oldSize;
    void* newPtr;

    PRIME_ASSERT(header[headerSignatureOffset] == headerSignature);

    oldSize = header[headerSizeOffset];

    newPtr = allocateAligned(newSize, newAlignment);
    if (!newPtr) {
        return NULL;
    }

    memcpy(newPtr, ptr, oldSize < newSize ? oldSize : newSize);
    freeAligned(ptr);

    return newPtr;
}

//
// Global memory management functions (declared in Common.h)
//

void* AllocateAligned(size_t size, size_t alignment)
{
    return MemoryManager::getGlobal()->allocateAligned(size, alignment);
}

void FreeAligned(void* ptr)
{
    MemoryManager::getGlobal()->freeAligned(ptr);
}

void* ReallocateAligned(void* ptr, size_t newSize, size_t newAlignment)
{
    return MemoryManager::getGlobal()->reallocateAligned(ptr, newSize, newAlignment);
}
}
