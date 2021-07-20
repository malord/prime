// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OPENSSLDIRECTHTTPCONNECTION_H
#define PRIME_OPENSSLDIRECTHTTPCONNECTION_H

#include "DirectHTTPConnection.h"
#include "OpenSSLContext.h"

#ifndef PRIME_NO_OPENSSL

namespace Prime {

/// Extend DirectHTTPConnectionFactory to support TLS/SSL connections.
class PRIME_PUBLIC OpenSSLDirectHTTPConnectionFactory : public DirectHTTPConnectionFactory {
public:
    explicit OpenSSLDirectHTTPConnectionFactory(int readTimeoutMilliseconds = -1, int writeTimeoutMilliseconds = -1);

    ~OpenSSLDirectHTTPConnectionFactory();

    bool init(Log* log);

    void close();

private:
    RefPtr<OpenSSLContext> _sslContext;
};
}

#endif // PRIME_NO_OPENSSL

#endif
