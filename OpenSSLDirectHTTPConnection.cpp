// Copyright 2000-2021 Mark H. P. Lord

#include "OpenSSLDirectHTTPConnection.h"

#ifndef PRIME_NO_OPENSSL

#include "OpenSSLSupport.h"

namespace Prime {

//
// OpenSSLDirectHTTPConnectionFactory
//

OpenSSLDirectHTTPConnectionFactory::OpenSSLDirectHTTPConnectionFactory(int readTimeoutMilliseconds, int writeTimeoutMilliseconds)
    : DirectHTTPConnectionFactory(readTimeoutMilliseconds, writeTimeoutMilliseconds)
{
}

OpenSSLDirectHTTPConnectionFactory::~OpenSSLDirectHTTPConnectionFactory()
{
    close();
}

bool OpenSSLDirectHTTPConnectionFactory::init(Log* log)
{
    if (!OpenSSLSupport::initSSL(log)) {
        return false;
    }

    _sslContext = PassRef(new OpenSSLContext);
    _sslContext->createClientContext(log);

    //This is forced to true until we have some way of verifying certificates
    _sslContext->setWarnAboutInvalidCertificates(false);

#ifdef PRIME_CXX11_STL
    auto sslContext = _sslContext;
    setSSLCallback([sslContext](Stream* s, Log* l) -> RefPtr<Stream> {
        return sslContext->connect(s, l);
    });
#else
    setSSLCallback(MethodCallback(_sslContext, &OpenSSLContext::connect));
#endif

    return true;
}

void OpenSSLDirectHTTPConnectionFactory::close()
{
    setSSLCallback(SSLCallback());
    _sslContext.release();
}
}

#endif // PRIME_NO_OPENSSL
