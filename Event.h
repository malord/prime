// Copyright 2000-2021 Mark H. P. Lord

// A Windows Event style threading primitive.

#ifndef PRIME_EVENT_H
#define PRIME_EVENT_H

#include "Lock.h"

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsEvent.h"

namespace Prime {
typedef WindowsEvent Event;
}

#else

#include "Emulated/EmulatedEvent.h"

namespace Prime {
typedef EmulatedEvent Event;
}

#endif

namespace Prime {

/// Implement the Lock interface using an Event.
template <typename Event = Prime::Event>
class EventLock : public Lock {
public:
    bool init(bool initiallySet, bool manualReset, Log* log, const char* debugName = NULL)
    {
        return _event.init(initiallySet, manualReset, log, debugName);
    }

    virtual void lock() { _event.lock(); }

    virtual void unlock() { _event.unlock(); }

private:
    Event _event;
};

}

#endif
