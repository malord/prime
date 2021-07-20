// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OPENSSLCONTEXT_H
#define PRIME_OPENSSLCONTEXT_H

#include "Config.h"
#include "Log.h"
#include "RefCounting.h"
#include "Stream.h"

#ifndef PRIME_NO_OPENSSL

typedef struct ssl_ctx_st SSL_CTX;

namespace Prime {

class PRIME_PUBLIC OpenSSLContext : public RefCounted {
public:
    OpenSSLContext();

    ~OpenSSLContext();

    bool createServerContext(const char* certificatePEM,
        const char* privateKeyPEM,
        const char* privateKeyPassphrase,
        Log* log);

    bool createClientContext(Log* log);

    void close();

    SSL_CTX* getOpenSSLContext() const { return _context; }

    void setWarnAboutInvalidCertificates(bool value) { _warnInvalidCertificate = value; }

    bool getWarnAboutInvalidCertificates() const { return _warnInvalidCertificate; }

    RefPtr<Stream> connect(Stream* stream, Log* log);

    RefPtr<Stream> accept(Stream* stream, Log* log);

private:
    bool useCertificate(const char* certificatePEM);
    bool usePrivateKey(const char* privateKeyPEM, const char* passphrase);

    static int pemPasswordCallback(char* buf, int size, int rwflag, void* userdata);

    SSL_CTX* _context;
    bool _warnInvalidCertificate;
};
}

#endif // PRIME_NO_OPENSSL

#endif
