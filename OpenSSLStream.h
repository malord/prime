// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OPENSSLSTREAM_H
#define PRIME_OPENSSLSTREAM_H

#include "OpenSSLContext.h"
#include "SocketStream.h"

#ifndef PRIME_NO_OPENSSL

typedef struct ssl_st SSL;

namespace Prime {

/// A NetworkStream that filters all I/O through an OpenSSLContext.
class PRIME_PUBLIC OpenSSLStream : public NetworkStream {
public:
    OpenSSLStream();

    ~OpenSSLStream();

    /// clientContext is retained.
    bool connect(OpenSSLContext* clientContext, SocketStream* underlyingStream, Log* log);

    /// serverContext is retained.
    bool accept(OpenSSLContext* serverContext, SocketStream* underlyingStream, Log* log);

    // NetworkStream implementation.
    virtual void setReadTimeout(int readTimeout) PRIME_OVERRIDE;
    virtual int getReadTimeout() const PRIME_OVERRIDE;
    virtual void setWriteTimeout(int writeTimeout) PRIME_OVERRIDE;
    virtual int getWriteTimeout() const PRIME_OVERRIDE;
    virtual WaitResult waitRead(int milliseconds, Log* log) PRIME_OVERRIDE;
    virtual WaitResult waitWrite(int milliseconds, Log* log) PRIME_OVERRIDE;

    // Stream implementation.
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readSome(void* buffer, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeSome(const void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual Stream* getUnderlyingStream() const PRIME_OVERRIDE { return _underlyingStream; }

    SocketStream* getUnderlyingSocketStream() const { return _underlyingStream; }

    bool hasPending() const;

private:
    void closeSSL();

    RefPtr<SocketStream> _underlyingStream;
    RefPtr<OpenSSLContext> _context;

    SSL* _ssl;

    PRIME_UNCOPYABLE(OpenSSLStream);
};
}

#endif // PRIME_NO_OPENSSL

#endif
