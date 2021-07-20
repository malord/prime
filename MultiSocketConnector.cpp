// Copyright 2000-2021 Mark H. P. Lord

#include "MultiSocketConnector.h"

namespace Prime {

MultiSocketConnector::MultiSocketConnector(int readTimeout, int writeTimeout)
    : SocketConnector(readTimeout, writeTimeout)
{
}

void MultiSocketConnector::add(SocketConnector* connector)
{
    _connectors.push_back(connector);
}

RefPtr<NetworkStream> MultiSocketConnector::connect(const char* hostname, int defaultPort, Log* log)
{
    return _connectors[rand() % _connectors.size()]->connect(hostname, defaultPort, log);
}
}
