// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DIRECTSOCKETCONNECTOR_H
#define PRIME_DIRECTSOCKETCONNECTOR_H

#include "SocketConnector.h"

namespace Prime {

class PRIME_PUBLIC DirectSocketConnector : public SocketConnector {
    PRIME_DECLARE_UID_CAST(SocketConnector, 0xfec7f74e, 0x1cdb45d7, 0xbb539725, 0xe7641917)

public:
    explicit DirectSocketConnector(int readTimeoutMilliseconds = -1, int writeTimeoutMilliseconds = -1);

    virtual RefPtr<NetworkStream> connect(const char* hostname, int defaultPort, Log* log) PRIME_OVERRIDE;
};
}

#endif
