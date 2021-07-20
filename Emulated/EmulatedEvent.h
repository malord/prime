// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_EMULATED_EMULATEDEVENT_H
#define PRIME_EMULATED_EMULATEDEVENT_H

#include "../Condition.h"
#include "../Mutex.h"

namespace Prime {

/// Emulate a Windows Event threading primitive (minus PulseEvent) using a recursive mutex and a
/// condition variable.
class PRIME_PUBLIC EmulatedEvent {
public:
    EmulatedEvent() { }

    EmulatedEvent(bool initiallySet, bool manualReset, Log* log, const char* debugName = NULL)
    {
        PRIME_EXPECT(init(initiallySet, manualReset, log, debugName));
    }

    ~EmulatedEvent() { close(); }

    bool init(bool initiallySet, bool manualReset, Log* log, const char* debugName = NULL);

    void close();

    bool isInitialised() { return _condition.isInitialised(); }

    void set();

    /// Wait for the event to be set.
    void wait();

    void reset();

    bool tryWait(int milliseconds = 0);

    void unlock() { set(); }

    void lock() { wait(); }

    bool tryLock(int milliseconds = 0) { return tryWait(milliseconds); }

    // bool isSet() const { return _set; }

    // No pulseEvent() - use a condition variable

private:
    RecursiveMutex _mutex;
    Condition _condition;

    bool _set;
    bool _manualReset;
};
}

#endif
