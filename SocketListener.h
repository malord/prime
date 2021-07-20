// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SOCKETLISTENER_H
#define PRIME_SOCKETLISTENER_H

#include "PrefixLog.h"
#include "SignalSocket.h"
#include "Socket.h"
#include "UnownedPtr.h"

namespace Prime {

/// Accepts connection from a listening Socket.
class PRIME_PUBLIC SocketListener : public RefCounted {
public:
    SocketListener();

    ~SocketListener();

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _retryBindForSeconds(30)
            , _retryAfterMilliseconds(250)
            , _defaultPort(80)
        {
        }

        Options& setRetryBindForSeconds(int value)
        {
            _retryBindForSeconds = value;
            return *this;
        }
        int getRetryBindForSeconds() const { return _retryBindForSeconds; }

        Options& setRetryAfterMilliseconds(int value)
        {
            _retryAfterMilliseconds = value;
            return *this;
        }
        int getRetryAfterMilliseconds() const { return _retryAfterMilliseconds; }

        Options& setCloseSignal(SignalSocket* value)
        {
            _closeSignal = value;
            return *this;
        }
        SignalSocket* getCloseSignal() const { return _closeSignal; }

        Options& setDefaultPort(int value)
        {
            _defaultPort = value;
            return *this;
        }
        int getDefaultPort() const { return _defaultPort; }

    private:
        int _retryBindForSeconds;
        int _retryAfterMilliseconds;
        UnownedPtr<SignalSocket> _closeSignal;
        int _defaultPort;
    };

    /// Start listening for connections.
    bool init(const char* address, const Options& options, Log* log, std::vector<std::string>* addresses = NULL);

    /// Set a SignalSocket which if signalled causes accept to immediately return.
    void setCloseSignal(SignalSocket* closeSignal) { _socket.setCloseSignal(closeSignal); }

    struct Connection {
        Socket socket;
        SocketAddress address;

        Connection()
        {
        }

#ifdef PRIME_COMPILER_RVALUEREF
        Connection(Connection&& move)
            : socket(std::move(move.socket))
            , address(std::move(move.address))
        {
        }

        Connection& operator=(Connection&& move)
        {
            socket = std::move(move.socket);
            address = std::move(move.address);
            return *this;
        }
#endif
    };

    /// Returns a member of the Socket:WaitResult enumeration.
    Socket::WaitResult accept(Connection& connection, int timeoutInMilliseconds,
        const Socket::Options& socketOptions = Socket::Options());

    /// Returns true if a connection has been made, false on error, otherwise loops and waits.
    bool accept(Connection& connection, const Socket::Options& socketOptions = Socket::Options());

    /// Close the socket and stop listening.
    void close();

    bool isClosed() const { return !_socket.isCreated(); }

    const SocketAddress& getLocalAddress() const { return _localAddress; }

private:
    Socket _socket;
    PrefixLog _log;
    SocketAddress _localAddress;
};
}

#endif
