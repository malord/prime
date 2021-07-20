// Copyright 2000-2021 Mark H. P. Lord

#include "SOCKS5SocketConnector.h"
#include "Convert.h"
#include "NumberParsing.h"
#include "SOCKS5Stream.h"
#include "SocketAddressParser.h"
#include "SocketStream.h"

namespace Prime {

PRIME_DEFINE_UID_CAST(SOCKS5SocketConnector)

SOCKS5SocketConnector::SOCKS5SocketConnector(int readTimeoutMilliseconds, int writeTimeoutMilliseconds)
    : SocketConnector(readTimeoutMilliseconds, writeTimeoutMilliseconds)
{
}

void SOCKS5SocketConnector::init(const SocketAddress& proxyAddress,
    StringView username, StringView password, SOCKSVersion socksVersion)
{
    _proxyUsername.assign(username.begin(), username.end());
    _proxyPassword.assign(password.begin(), password.end());
    _proxyAddress = proxyAddress;
    _socksVersion = socksVersion;
}

bool SOCKS5SocketConnector::init(const char* socks5,
    StringView username, StringView password, Log* log, SOCKSVersion socksVersion)
{
    // Allow a single int to be specified as the server address.
    int port;
    if (StringToInt(socks5, port)) {
        return init(MakeString("localhost:", port).c_str(), username, password, log, socksVersion);
    }

    SocketAddressParser sap;
    if (!sap.parse(socks5)) {
        log->error(PRIME_LOCALISE("Invalid proxy address."));
        return false;
    }

    SocketAddress proxyAddress;
    if (!sap.configureSocketAddress(proxyAddress, 1080, log)) {
        log->error(PRIME_LOCALISE("Can't find proxy address."));
        return false;
    }

    init(proxyAddress, username, password, socksVersion);
    return true;
}

RefPtr<NetworkStream> SOCKS5SocketConnector::connect(const char* hostname, int defaultPort, Prime::Log* log)
{
    SocketAddressParser sap;
    if (!sap.parse(hostname)) {
        log->error(PRIME_LOCALISE("Invalid hostname: %s"), hostname);
        return NULL;
    }

    RefPtr<SOCKS5Stream> socks5 = PassRef(new SOCKS5Stream(getReadTimeout(), getWriteTimeout()));
    if (!socks5->init(_proxyAddress, _proxyUsername, _proxyPassword, log, _socksVersion)) {
        return NULL;
    }

    switch (sap.getResult()) {
    case SocketAddressParser::ResultHostName:
        if (!socks5->connect(sap.getHostName(), sap.getPort(defaultPort), log)) {
            return NULL;
        }
        break;

    case SocketAddressParser::ResultDottedIP4: {
        SocketAddress addr;
        const int* ipAddress = sap.getIP();
        addr.setIP4(ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3], sap.getPort(defaultPort));
        if (!socks5->connect(addr, log)) {
            return NULL;
        }
        break;
    }

    default:
        log->error(PRIME_LOCALISE("Invalid URL hostname/port"));
        return NULL;
    }

    return socks5;
}
}
