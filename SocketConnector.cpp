// Copyright 2000-2021 Mark H. P. Lord

#include "SocketConnector.h"

namespace Prime {

PRIME_DEFINE_UID_CAST_BASE(SocketConnector)

SocketConnector::SocketConnector(int readTimeoutMilliseconds, int writeTimeoutMilliseconds)
    : _readTimeout(readTimeoutMilliseconds)
    , _writeTimeout(writeTimeoutMilliseconds)
{
}

SocketConnector::~SocketConnector()
{
}
}
