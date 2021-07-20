// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SIGNALSOCKET_H
#define PRIME_SIGNALSOCKET_H

#include "RefCounting.h"
#include "Socket.h"

namespace Prime {

/// Provides a Socket where having data to read is a signal (e.g., "please quit").
/// Allows select() to be cancelled by a signal in a reliable, cross-platform way. Once initialised, can be
/// shared across threads.
class PRIME_PUBLIC SignalSocket : public RefCounted {
public:
    SignalSocket();

    ~SignalSocket();

    bool init(Log* log);

    void close();

    bool signal(Log* log);

    void signal();

    void clear();

    bool isSignalled() const { return _atomic.get() != 0; }

    Socket::WaitResult wait(int milliseconds, Log* log) const;

    Socket& getSocket() const { return _socket; }

private:
    mutable Socket _socket;
    mutable AtomicCounter _atomic;
};
}

#endif
