// Copyright 2000-2021 Mark H. P. Lord

#include "SignalSocket.h"

namespace Prime {

SignalSocket::SignalSocket()
    : _atomic(0)
{
}

SignalSocket::~SignalSocket()
{
}

bool SignalSocket::init(Log* log)
{
    _atomic.set(0);

    if (!_socket.createUDPIP4(log, Socket::Options())) {
        return false;
    }

    SocketAddress quitSocketAddress;
    quitSocketAddress.setIP4(SocketAddress::ip4Localhost, 0);
    if (!_socket.bind(quitSocketAddress, log)) {
        return false;
    }

    if (!_socket.setNonBlocking(true, log)) {
        return false;
    }

    return true;
}

void SignalSocket::close()
{
    _socket.close(Log::getNullLog());
}

void SignalSocket::signal()
{
    signal(Log::getGlobal());
}

bool SignalSocket::signal(Log* log)
{
    _atomic.set(1);

    SocketAddress quitSocketAddress;
    if (_socket.getLocalAddress(quitSocketAddress, log)) {
        _socket.sendTo(quitSocketAddress, "WAKE", 4, log);
        return true;
    }

    return false;
}

void SignalSocket::clear()
{
    _atomic.set(0);
    wait(0, Log::getNullLog()); // This will read everything from the socket
}

Socket::WaitResult SignalSocket::wait(int milliseconds, Log* log) const
{
    for (;;) {
        switch (_socket.waitRecv(milliseconds, log)) {
        case Socket::WaitResultOK:
            if (!_atomic.get()) {
                // We didn't signal this!
                char buf[128];
                SocketAddress address;
                _socket.recvFrom(address, buf, sizeof(buf), log);

                address.describe(buf, sizeof(buf));
                log->trace("Received spurious packet on signal socket from: %s", buf);
                continue;
            }
            return Socket::WaitResultOK;

        case Socket::WaitResultTimedOut:
            return Socket::WaitResultTimedOut;

        case Socket::WaitResultCancelled:
            // Uh oh - Socket has already dealt with EINTR so this must be something bad
            log->developerWarning("SignalSocket WaitCancelled");
            return Socket::WaitResultCancelled;
        }
    }
}
}
