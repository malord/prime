// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OPENSSLSUPPORT_H
#define PRIME_OPENSSLSUPPORT_H

#include "Config.h"
#include "Log.h"

#ifndef PRIME_NO_OPENSSL

typedef struct ssl_ctx_st SSL_CTX;

namespace Prime {

/// Initialise OpenSSL for the entire application.
class PRIME_PUBLIC OpenSSLSupport {
public:
    explicit OpenSSLSupport(Log* log = Log::getGlobal())
    {
        initSSL(log);
    }

    ~OpenSSLSupport()
    {
        closeSSL(Log::getGlobal());
    }

    static bool initSSL(Log* log);

    /// Only call this if you're sure nothing will still be using OpenSSL. Best practice would be to construct
    /// an OpenSSLSupport inside main and let RAII deal with it.
    static void closeSSL(Log* log);

    static bool isInitialised() { return _initialised; }

private:
    static bool _initialised;
    static bool _ready;
};
}

#endif // PRIME_NO_OPENSSL

#endif
