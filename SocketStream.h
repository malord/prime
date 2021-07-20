// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SOCKETSTREAM_H
#define PRIME_SOCKETSTREAM_H

#include "NetworkStream.h"
#include "Socket.h"

namespace Prime {

/// A NetworkStream implementation for stream sockets (e.g., TCP).
class PRIME_PUBLIC SocketStream : public NetworkStream {
    PRIME_DECLARE_UID_CAST(NetworkStream, 0x96946a0d, 0xd06b42f0, 0x8e7514b0, 0xb7d447a4)

public:
    typedef Socket::Handle Handle;

    explicit SocketStream(int readTimeoutMilliseconds = -1, int writeTimeoutMilliseconds = -1)
        : _readTimeout(readTimeoutMilliseconds)
        , _writeTimeout(writeTimeoutMilliseconds)
    {
    }

    // There's no constructor that takes a Socket because it might give the impression that
    // this SocketStream will close the Socket when in fact the Socket would. Use either
    // takeOwnership() or the move constructor.

    virtual ~SocketStream() { }

#ifdef PRIME_COMPILER_RVALUEREF
    explicit SocketStream(Socket&& socket, int readTimeoutMilliseconds = -1, int writeTimeoutMilliseconds = -1)
        : _socket(std::move(socket))
        , _readTimeout(readTimeoutMilliseconds)
        , _writeTimeout(writeTimeoutMilliseconds)
    {
    }
#endif

    Socket& accessSocket()
    {
        return _socket;
    }

    Handle getHandle() const { return _socket.getHandle(); }

    /// Set our socket handle and specify whether it should be closed when this object is destructed or another
    /// handle is assigned.
    void setHandle(Handle handle, bool shouldClose) { _socket.setHandle(handle, shouldClose); }

    /// Move the specified Socket (and all its state) in to our Socket. "from" keeps the socket handle but loses
    /// responsibility for closing it. In C++11 and later, you can use the move constructor.
    void takeOwnership(Socket& from) { _socket.takeOwnership(from); }

    /// Returns true if we have a socket.
    bool isCreated() const { return _socket.isCreated(); }

    /// Returns true if this is a bad socket address.
    bool operator!() const { return !_socket.isCreated(); }

    /// Detach the socket handle from this object and return it.
    Handle detach() { return _socket.detach(); }

    // NetworkStream implementation.
    virtual void setReadTimeout(int readTimeout) PRIME_OVERRIDE { _readTimeout = readTimeout; }
    virtual int getReadTimeout() const PRIME_OVERRIDE { return _readTimeout; }
    virtual void setWriteTimeout(int writeTimeout) PRIME_OVERRIDE { _writeTimeout = writeTimeout; }
    virtual int getWriteTimeout() const PRIME_OVERRIDE { return _writeTimeout; }
    virtual WaitResult waitRead(int milliseconds, Log* log) PRIME_OVERRIDE;
    virtual WaitResult waitWrite(int milliseconds, Log* log) PRIME_OVERRIDE;

    void setBothTimeouts(int milliseconds)
    {
        setReadTimeout(milliseconds);
        setWriteTimeout(milliseconds);
    }

    // Stream implementation.
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readSome(void* buffer, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeSome(const void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual bool copyFrom(Stream* source, Log* sourceLog, Offset length, Log* destLog, size_t bufferSize = 0,
        void* buffer = NULL) PRIME_OVERRIDE;

private:
    static NetworkStream::WaitResult mapWaitResult(Socket::WaitResult socketWaitResult)
    {
        return (NetworkStream::WaitResult)(socketWaitResult);
    }

    Socket _socket;

    int _readTimeout;
    int _writeTimeout;

    PRIME_UNCOPYABLE(SocketStream);
};
}

#endif
