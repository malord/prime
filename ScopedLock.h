// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SCOPEDLOCK_H
#define PRIME_SCOPEDLOCK_H

#include "Config.h"

namespace Prime {

//
// ScopedLockBridge
//

/// Default Bridge for ScopedLock.
template <typename Lockable>
class ScopedLockBridge {
public:
    static void lock(Lockable* lockable) { lockable->lock(); }

    static void unlock(Lockable* lockable) { lockable->unlock(); }
};

//
// ScopedLock
//

/// A ScopedLock locks a threading primitive for as long as it exists (it unlocks when destructed).
/// The default Bridge, ScopedLockBridge, expects the type to have lock() and unlock() methods. By creating a
/// new Bridge type, other method names can be used.
template <typename Lockable, typename Bridge = ScopedLockBridge<Lockable>>
class ScopedLock {
public:
    /// Magic type used for ScopedLock's DoNotLock constructor.
    enum DoNotLockType { DoNotLock };

    ScopedLock()
        : _lockable(NULL)
    {
    }

    /// Immediately lock the specified object.
    ScopedLock(Lockable* lockable)
        : _lockable(lockable)
    {
        if (lockable) {
            Bridge::lock(lockable);
        }
    }

    /// Assign an object but don't lock it. It will be unlocked by the destructor.
    ScopedLock(Lockable* dontLock, DoNotLockType)
        : _lockable(dontLock)
    {
    }

    ~ScopedLock()
    {
        if (_lockable) {
            Bridge::unlock(_lockable);
        }
    }

    /// Get the object we're managing.
    Lockable* getLockable() const { return _lockable; }

    /// Try to lock the specified lock. Returns false if it wasn't locked and doesn't attach the lock to this
    /// object.
    bool tryLock(Lockable* lockable)
    {
        PRIME_ASSERT(!_lockable);

        if (!lockable->tryLock()) {
            return false;
        }

        _lockable = lockable;
        return true;
    }

    /// Lock the specified lock and attach it to this object.
    void lock(Lockable* lockable)
    {
        PRIME_ASSERT(!_lockable);
        Bridge::lock(lockable);
        _lockable = lockable;
    }

    /// Assign a different object to this lock, but don't lock it.
    void attach(Lockable* lockable)
    {
        PRIME_ASSERT(!_lockable);
        _lockable = lockable;
    }

    /// Unlock the object we're managing and detach it from this lock.
    void unlock()
    {
        PRIME_ASSERT(_lockable);
        Bridge::unlock(_lockable);
        _lockable = NULL;
    }

    /// Detach the object without unlocking it.
    void detach() { _lockable = NULL; }

    bool isLocked() const { return _lockable != NULL; }

private:
    Lockable* _lockable;

    PRIME_UNCOPYABLE(ScopedLock);
};

//
// ScopedReadLock
//

/// Bridge for ScopedReadLock.
template <typename Lockable>
class ScopedLockReadBridge {
public:
    static void lock(Lockable* lockable) { lockable->lockRead(); }

    static void unlock(Lockable* lockable) { lockable->unlockRead(); }
};

/// Locks a read/write lock for reading and unlocks it once destructed.
template <typename Lockable>
class ScopedReadLock : public ScopedLock<Lockable, ScopedLockReadBridge<Lockable>> {
public:
    typedef ScopedLock<Lockable, ScopedLockReadBridge<Lockable>> Super;
    typedef typename Super::DoNotLockType DoNotLockType;

    ScopedReadLock()
    {
    }

    /// Immediately lock the specified object.
    ScopedReadLock(Lockable* lockable)
        : Super(lockable)
    {
    }

    /// Assign an object but don't lock it. It will be unlocked by the destructor.
    ScopedReadLock(Lockable* dontLock, DoNotLockType)
        : Super(dontLock, Super::DoNotLock)
    {
    }

    PRIME_UNCOPYABLE(ScopedReadLock);
};

//
// ScopedWriteLock
//

/// Bridge for ScopedWriteLock.
template <typename Lockable>
class ScopedLockWriteBridge {
public:
    static void lock(Lockable* lockable) { lockable->lockWrite(); }

    static void unlock(Lockable* lockable) { lockable->unlockWrite(); }
};

/// Locks a read/write lock for writing and unlocks it once destructed.
template <typename Lockable>
class ScopedWriteLock : public ScopedLock<Lockable, ScopedLockWriteBridge<Lockable>> {
public:
    typedef ScopedLock<Lockable, ScopedLockWriteBridge<Lockable>> Super;
    typedef typename Super::DoNotLockType DoNotLockType;

    ScopedWriteLock()
    {
    }

    /// Immediately lock the specified object.
    ScopedWriteLock(Lockable* lockable)
        : Super(lockable)
    {
    }

    /// Assign an object but don't lock it. It will be unlocked by the destructor.
    ScopedWriteLock(Lockable* dontLock, DoNotLockType)
        : Super(dontLock, Super::DoNotLock)
    {
    }

    PRIME_UNCOPYABLE(ScopedWriteLock);
};

}

#endif
