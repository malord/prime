// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SOCKS5STREAM_H
#define PRIME_SOCKS5STREAM_H

#include "Config.h"
#include "SocketAddress.h"
#include "SocketStream.h"

namespace Prime {

enum SOCKSVersion {
    SOCKSVersionAuto,
    SOCKSVersion4,
    SOCKSVersion4A,
    SOCKSVersion5
};

/// Communicate via SOCKS5 protocol.
class SOCKS5Stream : public SocketStream {
    PRIME_DECLARE_UID_CAST(SocketStream, 0xe33331ea, 0x382443df, 0xaa84e431, 0x3e3602d2)

public:
    explicit SOCKS5Stream(int readTimeoutMilliseconds = -1, int writeTimeoutMilliseconds = -1)
        : SocketStream(readTimeoutMilliseconds, writeTimeoutMilliseconds)
    {
    }

    ~SOCKS5Stream();

    bool init(const SocketAddress& addr, StringView username, StringView password, Log* log,
        SOCKSVersion socksVersion = SOCKSVersionAuto);

    virtual bool connect(const char* hostname, int port, Log* log);
    virtual bool connect(const SocketAddress& addr, Log* log);

private:
    enum Command {
        CommandConnect = 1u,
        CommandBind = 2u
    };

    bool initSOCKS5(Command command, const SocketAddress* addr, const char* hostname, int port, Log* log);

    SocketAddress _proxyAddress;
    std::string _username;
    std::string _password;
    SOCKSVersion _socksVersion;

    PRIME_UNCOPYABLE(SOCKS5Stream);
};
}

#endif
