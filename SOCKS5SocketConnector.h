// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SOCKS5SOCKETCONNECTOR_H
#define PRIME_SOCKS5SOCKETCONNECTOR_H

#include "SOCKS5Stream.h"
#include "SocketAddress.h"
#include "SocketConnector.h"

namespace Prime {

class PRIME_PUBLIC SOCKS5SocketConnector : public SocketConnector {
    PRIME_DECLARE_UID_CAST(SocketConnector, 0x19290674, 0x34cf4eee, 0x92bfd413, 0x4a00db7e)

public:
    explicit SOCKS5SocketConnector(int readTimeoutMilliseconds = -1, int writeTimeoutMilliseconds = -1);

    void init(const SocketAddress& proxyAddress, StringView username, StringView password, SOCKSVersion socksVersion = SOCKSVersionAuto);

    bool init(const char* proxyAddress, StringView username, StringView password, Log* log, SOCKSVersion socksVersion = SOCKSVersionAuto);

    virtual RefPtr<NetworkStream> connect(const char* hostname, int defaultPort, Log* log) PRIME_OVERRIDE;

private:
    SocketAddress _proxyAddress;
    SOCKSVersion _socksVersion;
    std::string _proxyUsername;
    std::string _proxyPassword;
};
}

#endif
