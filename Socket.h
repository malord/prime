// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SOCKET_H
#define PRIME_SOCKET_H

#include "RefCounting.h"
#include "SocketAddress.h"

namespace Prime {

class SignalSocket;

/// Encapsulates a socket.
class PRIME_PUBLIC Socket {
public:
    /// Platform's socket handle type.
    typedef SocketSupport::Handle Handle;

    /// Result of a wait operation.
    enum WaitResult {
        /// The wait operation was aborted, possibly because the socket was closed.
        WaitResultCancelled,

        /// Data became available.
        WaitResultOK,

        /// Timeout reached.
        WaitResultTimedOut
    };

    /// Options for the create*() and accept() methods.
    class PRIME_PUBLIC Options {
    public:
        Options()
            : _childProcessInherit(false)
        {
        }

        /// If not set, socket handles will be set to not be inherited by child processes.
        Options& setChildProcessInherit(bool value = true)
        {
            _childProcessInherit = value;
            return *this;
        }
        bool getChildProcessInherit() const { return _childProcessInherit; }

    private:
        bool _childProcessInherit;
    };

    Socket()
        : _handle(SocketSupport::invalidHandle)
        , _shouldClose(false)
        , _shouldRetry(true)
        , _lastError(0)
    {
    }

    /// Initialise with a socket handle.
    explicit Socket(Handle existingHandle, bool closeWhenDone = true)
        : _handle(existingHandle)
        , _shouldClose(closeWhenDone)
        , _shouldRetry(true)
        , _lastError(0)
    {
    }

#ifdef PRIME_COMPILER_RVALUEREF
    Socket(Socket&& move)
        : _handle(move._handle)
        , _shouldClose(move._shouldClose)
        , _shouldRetry(move._shouldRetry)
        , _lastError(move._lastError)
        , _closeSignal(move._closeSignal)
    {
        move._shouldClose = false;
        move._handle = SocketSupport::invalidHandle;
    }

    Socket& operator=(Socket&& move)
    {
        takeOwnership(move);
        return *this;
    }
#endif

    ~Socket()
    {
        close(Log::getNullLog());
    }

    /// Return the socket handle.
    Handle getHandle() const { return _handle; }

    /// Set our socket handle.
    void setHandle(Handle existingHandle, bool closeWhenDone)
    {
        close(Log::getNullLog());

        _handle = existingHandle;
        _shouldClose = closeWhenDone;
    }

    /// Set a SignalSocket which if set, signifies to waitRecv() and waitSend() that this socket should be
    /// considered closed.
    void setCloseSignal(SignalSocket* closeSignal);

    SignalSocket* getCloseSignal() const;

    Socket* getCloseSignalSocket() const;

    /// Close the socket, if we have one. If an error occurs, returns false. If we don't have an open socket,
    /// returns true.
    bool close(Log* log);

    /// Returns true if we have a socket.
    bool isCreated() const { return _handle != SocketSupport::invalidHandle; }

    /// Returns true if this is a bad socket address.
    bool operator!() const { return _handle == SocketSupport::invalidHandle; }

    /// Detach the socket handle from this object.
    Handle detach()
    {
        Handle detached = _handle;
        _handle = SocketSupport::invalidHandle;
        _shouldClose = false;
        return detached;
    }

    /// Create a socket.
    bool create(int family, int type, int protocol, Log* log, const Options& options);

    /// Create a new IP4 socket.
    bool createTCPIP4(Log* log, const Options& options = Options());

    /// Create a new UDP IP4 socket.
    bool createUDPIP4(Log* log, const Options& options = Options());

    /// Create a socket for communication with the specified address.
    bool createForAddress(const SocketAddress& address, int socketType, int protocol, Log* log, const Options& options);

    /// Connect to the specified address.
    bool connect(const SocketAddress& address, Log* log);

    /// Like `connect`, but use the receive timeout.
    bool connect(const SocketAddress& address, int milliseconds, Log* log);

    /// `connect` for a socket which has already been set non-blocking.
    bool nonBlockingConnect(const SocketAddress& address, int milliseconds, Log* log);

    /// Wait, up to the specified number of milliseconds, for data to become available to read. Returns a member
    /// of the WaitResult enumeration. Specify -1 for milliseconds to wait forever.
    WaitResult waitRecv(int milliseconds, Log* log);

    /// Wait, up to the specified number of milliseconds, for space to become available in the send buffer.
    /// Returns a member of the WaitResult enumeration. Specify -1 for milliseconds to wait forever.
    WaitResult waitSend(int milliseconds, Log* log);

    struct SelectSocket {
        Socket* socket;
        bool isSet; // set on exit
    };

    /// Wait for one of a number of sockets. Can be called on any of the Sockets in the arrays, and only uses
    /// "this" to read error handling options. Ignores the SignalSocket set with setCloseSignal().
    WaitResult select(int milliseconds, SelectSocket* reads, SelectSocket* writes, SelectSocket* errors, Log* log);

    /// Read up to the requested number of bytes. Returns the number of bytes read, which will be 0 if the
    /// connection has been closed. On error, returns -1.
    ptrdiff_t recv(void* buffer, size_t request, Log* log);

    /// Send up the requested number of bytes. Returns the number of bytes written, which will be 0 if the
    /// connection has been closed. On error, returns -1.
    ptrdiff_t send(const void* buffer, size_t request, Log* log);

    /// Repeatedly call send() until all of data has been sent. On error, or if the remote socket was closed
    /// before all the data was written, returns false.
    bool sendAll(const void* data, size_t length, Log* log);

    /// Bind this socket to the specified address. This determines which adapters we will allow connections on.
    bool bind(const SocketAddress& address, Log* log);

    /// Begin waiting for connections. "queue" specifies the maximum number of connections that can be waiting at
    /// any given time. If -1, the system maximum is requested. If the socket is unable to listen for connections,
    /// returns false.
    bool listen(Log* log, int queue = -1);

    /// Accept the next connection on this socket. On success, assigns the socket handle to socket and puts the
    /// address of the remote socket in addr. On failure, returns false. To wait for a connection, use waitRecv().
    /// "options" is a combination of the members of the Options enumeration, or zero.
    bool accept(Socket& socket, SocketAddress& addr, Log* log, const Options& options);

    /// Read a packet on a connectionless socket, setting *addr to the address of the remote socket from which the
    /// packet was received. Returns the number of bytes read, -1 on error.
    ptrdiff_t recvFrom(SocketAddress& addr, void* buffer, size_t bufferSize, Log* log);

    /// Write a packet on a connectionless socket. Returns the number of bytes actually sent or -1 on error.
    ptrdiff_t sendTo(const SocketAddress& toAddress, const void* buffer, size_t byteCount, Log* log);

    /// Return the address of the other side of the connection.
    bool getRemoteAddress(SocketAddress& addr, Log* log);

    /// Return the address this socket is bound to.
    bool getLocalAddress(SocketAddress& addr, Log* log);

    /// Set the non-blocking option on this socket. Returns false on error.
    bool setNonBlocking(bool value, Log* log);

    /// Enable broadcast address on this socket. Returns false on error.
    bool setBroadcast(bool value, Log* log);

    /// Enable or disable SO_REUSEADDR, allowing a socket to reuse an address formerly in use by another process
    /// that is still in TIME_WAIT state.
    bool setReuseAddress(bool value, Log* log);

    /// Specify whether or not interrupted system calls should be retried. Defaults to true.
    void setRetry(bool retry) { _shouldRetry = retry; }

    /// Returns the last error code.
    SocketSupport::ErrorCode getLastError() const { return _lastError; }

    void takeOwnership(Socket& from);

private:
    /// If the last system call failed due to an interrupt, returns true if the caller should retry the operation.
    /// In all other cases, logs the error and sets _lastError and returns false.
    bool handleError(SocketSupport::ErrorCode errorCode, Log* log);

    /// handleError() specialised for send()/recv() - can instruct send()/recv() to return zero instead of error.
    bool handleSendRecvError(Log* log, bool& returnZero);

    /// Calls handleError with the result of SocketSupport::getLastSocketError().
    bool handleError(Log* log);

    Handle _handle;
    bool _shouldClose;

    volatile bool _shouldRetry;
    SocketSupport::ErrorCode _lastError;

    /// We don't have the complete type of SignalSocket at this point, so use RefCounted.
    RefPtr<RefCounted> _closeSignal;

    /// No copy constructor. Must explicitly copy the socket to ensure ownership semantics are correctly handled.
    Socket(const Socket&) { }
    Socket& operator=(const Socket&) { return *this; }
};
}

#endif
