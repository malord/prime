// Copyright 2000-2021 Mark H. P. Lord

#include "DirectSocketConnector.h"
#include "SocketAddressParser.h"
#include "SocketStream.h"

namespace Prime {

PRIME_DEFINE_UID_CAST(DirectSocketConnector)

DirectSocketConnector::DirectSocketConnector(int readTimeoutMilliseconds, int writeTimeoutMilliseconds)
    : SocketConnector(readTimeoutMilliseconds, writeTimeoutMilliseconds)
{
}

RefPtr<NetworkStream> DirectSocketConnector::connect(const char* hostname, int defaultPort, Prime::Log* log)
{
    SocketAddressParser sap;
    if (!sap.parse(hostname)) {
        log->error(PRIME_LOCALISE("Invalid hostname: %s"), hostname);
        return NULL;
    }

    RefPtr<SocketStream> socketStream = PassRef(new SocketStream(getReadTimeout(), getWriteTimeout()));

    SocketAddress addr;
    if (!sap.createAndConnectSocket(socketStream->accessSocket(), sap.getPort(defaultPort), SOCK_STREAM, IPPROTO_TCP,
            Socket::Options(), getReadTimeout(), log, &addr)) {
        // log->error(PRIME_LOCALISE("Couldn't connect to server: %s"), hostWithPort.c_str());
        return NULL;
    }

    return socketStream;
}

}
