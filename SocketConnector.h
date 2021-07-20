// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SOCKETCONNECTOR_H
#define PRIME_SOCKETCONNECTOR_H

#include "NetworkStream.h"

namespace Prime {

/// Creates NetworkStreams to connect to a host name and port. DirectSocketConnector and SOCKS5SocketConnector
/// exist at the time of writing, although it would be possible to have a SocketConnector which selected a
/// SocketConnector based on domain.
class PRIME_PUBLIC SocketConnector : public RefCounted {
    PRIME_DECLARE_UID_CAST_BASE(0x5c8b9cd7, 0x0e364e7b, 0x94f9f7f1, 0x3775a3cd)

public:
    explicit SocketConnector(int readTimeoutMilliseconds = -1, int writeTimeoutMilliseconds = -1);

    virtual ~SocketConnector();

    /// hostname can be a dotted IP or a name to be resolved and can contain a port, e.g., "example.com:443".
    /// defaultPort is used if no port is present in hostname.
    virtual RefPtr<NetworkStream> connect(const char* hostname, int defaultPort, Log* log) = 0;

    void setReadTimeout(int milliseconds) { _readTimeout = milliseconds; }
    int getReadTimeout() const { return _readTimeout; }

    void setWriteTimeout(int milliseconds) { _writeTimeout = milliseconds; }
    int getWriteTimeout() const { return _writeTimeout; }

private:
    int _readTimeout;
    int _writeTimeout;
};
}

#endif
