// Copyright 2000-2021 Mark H. P. Lord

#include "SocketListener.h"
#include "Clocks.h"
#include "Convert.h"
#include "SocketAddressParser.h"
#include "Templates.h"

namespace Prime {

SocketListener::SocketListener()
{
}

SocketListener::~SocketListener()
{
}

void SocketListener::close()
{
    _socket.close(_log);
}

bool SocketListener::init(const char* address, const Options& options, Log* log,
    std::vector<std::string>* addresses)
{
    _log.setLog(log);
    _log.setPrefix(address);

    SocketAddressParser sap;
    if (!sap.parse(address)) {
        _log.error(PRIME_LOCALISE("Can't parse server address."));
        return false;
    }

    SocketAddress addr;
    if (!sap.configureSocketAddress(addr, options.getDefaultPort(), SOCK_STREAM, NULL, IPPROTO_TCP, NULL, _log)) {
        _log.error(PRIME_LOCALISE("Can't configure socket address."));
        return false;
    }

    int port = addr.getPort();
    if (port >= 0 && addresses && addr.isAny()) {
        SocketAddress::getAllInterfaceAddresses(*addresses, port, log);
    }

    if (!_socket.createForAddress(addr, SOCK_STREAM, IPPROTO_TCP, _log, Socket::Options())) {
        _log.error(PRIME_LOCALISE("Can't create socket."));
        return false;
    }

    // The socket needs to be non-blocking to deal with UNIX handle inheritance issues (see the comment in accept()).
    if (!_socket.setNonBlocking(true, _log)) {
        _log.error(PRIME_LOCALISE("Can't set socket to non-blocking."));
        return false;
    }

    _socket.setCloseSignal(options.getCloseSignal());

    _socket.setReuseAddress(true, PrefixLog(_log, "SO_REUSEADDR"));

    const int retryAfterMilliseconds = options.getRetryAfterMilliseconds();
    int retryBindCount = (int)((options.getRetryBindForSeconds() * 1000 + retryAfterMilliseconds) / retryAfterMilliseconds);
    int retryBindRemaining = retryBindCount;

    for (;;) {
        if (_socket.bind(addr, _log)) {
            break;
        }

        if (_socket.getLastError() != SocketSupport::ErrorAddressInUse) {
            return false;
        }

        if (retryBindRemaining <= 0) {
            _log.error(PRIME_LOCALISE("Can't bind socket."));
            return false;
        }

        if (retryBindCount == retryBindRemaining) {
            _log.note(PRIME_LOCALISE("Will retry bind %d time(s)."), retryBindRemaining - 1);
        }

        --retryBindRemaining;

        Clock::sleepMilliseconds(retryAfterMilliseconds);
    }

    SocketAddress boundAddress;
    char description[128];
    if (_socket.getLocalAddress(boundAddress, PrefixLog(_log, "getLocalAddress"))) {
        if (boundAddress.describe(description, sizeof(description), true)) {
            _log.setPrefix(MakeString("Server ", description));
            if (addresses && addresses->empty()) {
                PushBackUnique(*addresses, description);
            }
        }

        _localAddress = boundAddress;
    } else {
        _localAddress = SocketAddress();
    }

    if (!_socket.listen(_log)) {
        _log.error(PRIME_LOCALISE("Can't set socket to listen."));
        return false;
    }

    return true;
}

Socket::WaitResult SocketListener::accept(Connection& connection, int timeoutInMilliseconds,
    const Socket::Options& socketOptions)
{
    // We can't just wait on accept() because on UNIX, Socket::accept() locks the global UnixCloseOnExec mutex
    // (which is used to prevent handles being inherited by child processes). To get around this, we wait for
    // something to become readable on the socket before calling accept(), but since multiple threads can be
    // waiting on the same socket there's a race condition where multiple threads wake up and then all try and
    // call accept(), which locks the UnixCloseOnExec mutex. The workaround is to use non-blocking sockets, and to
    // set the newly acquired socket to blocking to deal with platforms where non-blocking-ness is inherited.

    for (;;) {
        // TODO: adjust the timeout after each loop?
        Socket::WaitResult waitResult = _socket.waitRecv(timeoutInMilliseconds, _log);

        if (waitResult == Socket::WaitResultOK) {
            if (!_socket.isCreated()) {
                // Closed on the main thread.
                return Socket::WaitResultCancelled;
            }

            if (_socket.accept(connection.socket, connection.address, _log, socketOptions)) {
                connection.socket.setNonBlocking(false, _log);
                return Socket::WaitResultOK;
            } else {
                if (_socket.getLastError() == SocketSupport::ErrorWouldBlock) {
                    // Another thread beat us to it.
                    continue;
                }
                return Socket::WaitResultCancelled;
            }
        }

        return waitResult;
    }
}

bool SocketListener::accept(Connection& connection, const Socket::Options& socketOptions)
{
    return accept(connection, -1, socketOptions) == Socket::WaitResultOK;
}
}
