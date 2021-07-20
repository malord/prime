// Copyright 2000-2021 Mark H. P. Lord

#include "OpenSSLContext.h"

#ifndef PRIME_NO_OPENSSL

#include "OpenSSLStream.h"
#include "StringUtils.h"
#include <openssl/ssl.h>

namespace Prime {

OpenSSLContext::OpenSSLContext()
    : _context(NULL)
    , _warnInvalidCertificate(false)
{
}

OpenSSLContext::~OpenSSLContext()
{
    close();
}

bool OpenSSLContext::createServerContext(const char* certificatePEM, const char* privateKeyPEM,
    const char* privateKeyPassphrase, Log* log)
{
    close();

    const SSL_METHOD* meth = SSLv23_server_method();
    _context = SSL_CTX_new(meth);
    if (!_context) {
        close();
        log->error(PRIME_LOCALISE("Couldn't create SSL server context."));
        return false;
    }

    if (!useCertificate(certificatePEM)) {
        close();
        log->error(PRIME_LOCALISE("Couldn't initialise certificate."));
        return false;
    }

    if (!usePrivateKey(privateKeyPEM, privateKeyPassphrase)) {
        close();
        log->error(PRIME_LOCALISE("Couldn't initialise private key."));
        return false;
    }

    if (!SSL_CTX_check_private_key(_context)) {
        close();
        log->error(PRIME_LOCALISE("SSL server private key not valid."));
        return false;
    }

    return true;
}

bool OpenSSLContext::useCertificate(const char* certificatePEM)
{
    BIO* bio = BIO_new_mem_buf((void*)certificatePEM, -1);
    if (!bio) {
        return false;
    }

    bool success = false;

    while (X509* certificate = PEM_read_bio_X509(bio, NULL, 0, NULL)) {
        int result = SSL_CTX_use_certificate(_context, certificate);
        if (result != 1) {
            return false;
        }

        success = true;

        X509_free(certificate);
    }

    BIO_free(bio);

    return success;
}

bool OpenSSLContext::usePrivateKey(const char* privateKeyPEM, const char* passphrase)
{
    BIO* bio = BIO_new_mem_buf((void*)privateKeyPEM, -1);
    if (!bio) {
        return false;
    }

    RSA* rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, &OpenSSLContext::pemPasswordCallback, (void*)passphrase);
    if (!rsa) {
        BIO_free(bio);
        return false;
    }

    int result = SSL_CTX_use_RSAPrivateKey(_context, rsa);

    RSA_free(rsa);
    BIO_free(bio);

    return result == 1;
}

int OpenSSLContext::pemPasswordCallback(char* buf, int size, int rwflag, void* userdata)
{
    (void)rwflag;
    StringCopy(buf, (size_t)size, (const char*)userdata);
    return (int)strlen(buf);
}

bool OpenSSLContext::createClientContext(Log* log)
{
    close();

    const SSL_METHOD* meth = SSLv23_client_method();
    _context = SSL_CTX_new(meth);

    if (!_context) {
        close();
        log->error(PRIME_LOCALISE("Couldn't create SSL client context."));
        return false;
    }

    return true;
}

void OpenSSLContext::close()
{
    if (_context) {
        SSL_CTX_free(_context);
        _context = NULL;
    }
}

RefPtr<Stream> OpenSSLContext::connect(Stream* stream, Log* log)
{
    SocketStream* socketStream = UIDCast<SocketStream>(stream);
    if (!socketStream) {
        log->error(PRIME_LOCALISE("Only socket streams support SSL."));
        return NULL;
    }

    RefPtr<OpenSSLStream> sslStream = PassRef(new OpenSSLStream);

    if (!sslStream->connect(this, socketStream, log)) {
        return NULL;
    }

    return sslStream;
}

RefPtr<Stream> OpenSSLContext::accept(Stream* stream, Log* log)
{
    SocketStream* socketStream = UIDCast<SocketStream>(stream);
    if (!socketStream) {
        log->error(PRIME_LOCALISE("Only socket streams support SSL."));
        return NULL;
    }

    RefPtr<OpenSSLStream> sslStream = PassRef(new OpenSSLStream);

    if (!sslStream->accept(this, socketStream, log)) {
        return NULL;
    }

    return sslStream;
}
}

#endif // PRIME_NO_OPENSSL
