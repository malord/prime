// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MULTISOCKETCONNECTOR_H
#define PRIME_MULTISOCKETCONNECTOR_H

#include "SocketConnector.h"

namespace Prime {

/// Randomly connect via one of multiple SocketConnectors. Can be used to spread requests across multiple networks
/// or for fuzz testing.
class PRIME_PUBLIC MultiSocketConnector : public SocketConnector {
public:
    MultiSocketConnector(int readTimeout = -1, int writeTimeout = -1);

    void add(SocketConnector* connector);

    virtual RefPtr<NetworkStream> connect(const char* hostname, int defaultPort, Log* log) PRIME_OVERRIDE;

private:
    std::vector<RefPtr<SocketConnector>> _connectors;
};
}

#endif
