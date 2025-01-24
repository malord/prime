// Copyright 2000-2021 Mark H. P. Lord

#include "OpenSSLStream.h"

#ifndef PRIME_NO_OPENSSL

#include "OpenSSLSupport.h"
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace Prime {

OpenSSLStream::OpenSSLStream()
    : _ssl(NULL)
{
}

OpenSSLStream::~OpenSSLStream()
{
    closeSSL();
}

bool OpenSSLStream::connect(OpenSSLContext* clientContext, SocketStream* underlyingStream, Log* log)
{
    if (!OpenSSLSupport::initSSL(log)) {
        return false;
    }

    close(log);

    _underlyingStream = underlyingStream;
    _context = clientContext;

    Socket::Handle sd = underlyingStream->getHandle();

    _ssl = SSL_new(clientContext->getOpenSSLContext());
    if (!_ssl) {
        log->error(PRIME_LOCALISE("Unable to create SSL instance from client context."));
        return false;
    }

    // According to: http://openssl.6102.n7.nabble.com/Sockets-windows-64-bit-td36169.html
    // ...this cast to int is safe.
    SSL_set_fd(_ssl, (int)sd); // If we can remove this, we can be modified to operate on any kind of NetworkStream.
    int err = SSL_connect(_ssl);
    if (err == -1) {
        closeSSL();
        log->error(PRIME_LOCALISE("Connection to SSL server failed. err: %d errno: %d"), (int) SSL_get_error(_ssl, err), (int) errno);
        return false;
    }

    if (err != 1) {
        log->error(PRIME_LOCALISE("SSL handshake failed: %d err: %d errno: %d."), err, (int) SSL_get_error(_ssl, err), (int) errno);
        return false;
    }

    //log->trace("SSL connection using: %s\n", SSL_get_cipher(_ssl));

    X509* server_cert = SSL_get_peer_certificate(_ssl);
    if (!server_cert) {
        closeSSL();
        log->error(PRIME_LOCALISE("Did not receive SSL certificate from server."));
        return false;
    }

    if (clientContext->getWarnAboutInvalidCertificates() && SSL_get_verify_result(_ssl) != X509_V_OK) {
        log->warning(PRIME_LOCALISE("SSL certificate received from server may be invalid."));
    }

    /*
        char* str = X509_NAME_oneline(X509_get_subject_name(server_cert),0,0);
        if (! str) {
            X509_free(server_cert);
            closeSSL();
            log->error(PRIME_LOCALISE("Unable to get SSL certificate subject name."));
            return false;
        }
        log->trace("SSL certificate subject: %s\n", str);
        free(str);

        str = X509_NAME_oneline(X509_get_issuer_name (server_cert),0,0);
        if (! str) {
            X509_free(server_cert);
            closeSSL();
            log->error(PRIME_LOCALISE("Unable to get SSL certificate issuer name."));
            return false;
        }
        log->trace("SSL certificate issuer: %s", str);
        free(str);
*/

    /* We could do all sorts of certificate verification stuff here before
         deallocating the certificate. */

    X509_free(server_cert);

    return true;
}

bool OpenSSLStream::accept(OpenSSLContext* serverContext, SocketStream* underlyingStream, Log* log)
{
    close(log);

    _context = serverContext;
    _underlyingStream = underlyingStream;

    Socket::Handle sd = underlyingStream->getHandle();

    _ssl = SSL_new(serverContext->getOpenSSLContext());
    if (!_ssl) {
        closeSSL();
        log->error(PRIME_LOCALISE("Unable to create SSL server instance."));
    }

    // According to: http://openssl.6102.n7.nabble.com/Sockets-windows-64-bit-td36169.html
    // this cast to int is safe.
    SSL_set_fd(_ssl, (int)sd); // If we can remove this, we can be modified to operate on any kind of NetworkStream.
    int ret = SSL_accept(_ssl);
    if (ret <= 0) {
        log->error(PRIME_LOCALISE("SSL connection from client failed: %d err: %d."), ret, SSL_get_error(_ssl, ret));
        closeSSL();
        return false;
    }

    //log->trace("SSL connection using: %s", SSL_get_cipher(_ssl));
    return true;
}

void OpenSSLStream::setReadTimeout(int readTimeout)
{
    _underlyingStream->setReadTimeout(readTimeout);
}

int OpenSSLStream::getReadTimeout() const
{
    return _underlyingStream->getReadTimeout();
}

void OpenSSLStream::setWriteTimeout(int writeTimeout)
{
    _underlyingStream->setWriteTimeout(writeTimeout);
}

int OpenSSLStream::getWriteTimeout() const
{
    return _underlyingStream->getWriteTimeout();
}

NetworkStream::WaitResult OpenSSLStream::waitRead(int milliseconds, Log* log)
{
    int pending = SSL_pending(_ssl);

    if (pending > 0) {
        // log->trace("**** %d pending bytes", pending);
        return WaitResultOK;
    }

    return _underlyingStream->waitRead(milliseconds, log);
}

bool OpenSSLStream::hasPending() const
{
    return SSL_pending(_ssl) > 0;
}

NetworkStream::WaitResult OpenSSLStream::waitWrite(int milliseconds, Log* log)
{
    // It's possible that OpenSSL has space in its own buffers but that the socket itself cannot write.
    // Right now, I've decided to just wait until the socket buffers aren't full and ignore the OpenSSL buffer.
    return _underlyingStream->waitWrite(milliseconds, log);
}

void OpenSSLStream::closeSSL()
{
    if (_ssl) {
#if 0
                ScopedYieldThread yield;

                while (SSL_shutdown(_ssl) == 0)
                    {}
#else
        SSL_set_shutdown(_ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
#endif
        SSL_free(_ssl);
        _ssl = NULL;
    }

    _context.release();
}

bool OpenSSLStream::close(Log* log)
{
    closeSSL();

    if (_underlyingStream) {
        return _underlyingStream->close(log);
    }

    return true;
}

ptrdiff_t OpenSSLStream::readSome(void* buffer, size_t maximumBytes, Log* log)
{
    PRIME_ASSERT(_ssl);

    if (!waitReadTimeout(log)) {
        return -1;
    }

    ptrdiff_t bytesRead = SSL_read(_ssl, buffer, (int)maximumBytes);
    if (bytesRead < 0) {
        log->error(PRIME_LOCALISE("SSL socket read error."));
        return -1;
    }

    return (ptrdiff_t)bytesRead;
}

ptrdiff_t OpenSSLStream::writeSome(const void* memory, size_t maximumBytes, Log* log)
{
    PRIME_ASSERT(_ssl);

    if (!waitWriteTimeout(log)) {
        return -1;
    }

    ptrdiff_t bytesWritten = SSL_write(_ssl, memory, (int)maximumBytes);
    if (bytesWritten < 0) {
        log->error(PRIME_LOCALISE("SSL socket write error."));
        return -1;
    }

    return bytesWritten;
}
}

#endif // PRIME_NO_OPENSSL
