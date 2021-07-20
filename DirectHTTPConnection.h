// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DIRECTHTTPCONNECTION_H
#define PRIME_DIRECTHTTPCONNECTION_H

#include "HTTPConnection.h"
#include "Mutex.h"
#include "URL.h"
#ifndef PRIME_CXX11_STL
#include "Callback.h"
#endif
#include "SocketConnector.h"
#include "StreamBuffer.h"
#include <functional>
#include <map>

// Define this as zero if you don't want to pull in MultiStream
#ifndef PRIME_DIRECTHTTPCONNECTION_ENABLE_TRANSCRIPTS
#define PRIME_DIRECTHTTPCONNECTION_ENABLE_TRANSCRIPTS 1
#endif

namespace Prime {

/// An HTTPConnectionFactory whose HTTPConnection connects to the server using a SocketConnector, bypassing
/// any OS specific services.
class PRIME_PUBLIC DirectHTTPConnectionFactory : public HTTPConnectionFactory {
public:
    explicit DirectHTTPConnectionFactory(int readTimeoutMilliseconds = -1, int writeTimeoutMilliseconds = -1);

    enum { bufferSize = 65536u };

    virtual RefPtr<HTTPConnection> createConnection(const URLView& url, Log* log) PRIME_OVERRIDE;

    // Setting this to zero disables automatic redirects.
    int getMaxRedirects() const { return _maxRedirects; }
    virtual void setMaxRedirects(int value) PRIME_OVERRIDE { _maxRedirects = value; }

    // Setting this to zero disables automatic retries. A retry only occurs if there's an error while reading
    // headers.
    int getMaxRetries() const { return _maxRetries; }
    void setMaxRetries(int value) { _maxRetries = value; }

    void setSocketConnector(SocketConnector* connector);

    int getReadTimeout() const { return _connector->getReadTimeout(); }
    int getWriteTimeout() const { return _connector->getWriteTimeout(); }

/// The callback should wrap the raw Stream with an OpenSSLStream.
#ifdef PRIME_CXX11_STL
    typedef std::function<RefPtr<Stream>(Stream*, Log*)> SSLCallback;
#else
    typedef Callback2<RefPtr<Stream>, Stream*, Log*> SSLCallback;
#endif

    const SSLCallback& getSSLCallback() const
    {
        return _sslCallback;
    }
    void setSSLCallback(const SSLCallback& value) { _sslCallback = value; }

#if PRIME_DIRECTHTTPCONNECTION_ENABLE_TRANSCRIPTS
    void setTranscriptStream(Stream* transcriptStream)
    {
        _transcriptStream = transcriptStream;
    }
#endif

protected:
    class Connection;
    friend class Connection;

    virtual RefPtr<StreamBuffer> connect(const URLView& url, bool& isKeepAlive, Log* log);

    virtual void returnToPool(const URLView& url, StreamBuffer* stream);

private:
    struct Host {
        std::string host;
        std::string protocol;
        int port;

        Host();

        Host(const URLView& url);

        Host(const Host& copy);

#ifdef PRIME_COMPILER_RVALUEREF
        Host(Host&& move) PRIME_NOEXCEPT;
#endif

        bool operator<(const Host& rhs) const;

        bool operator==(const Host& rhs) const;

        PRIME_IMPLIED_COMPARISONS_OPERATORS(const Host&)
    };

    typedef std::multimap<Host, RefPtr<StreamBuffer>> KeepAlivesMap;
    KeepAlivesMap _keepAlives;

    RecursiveMutex _mutex;

    int _maxRedirects;
    int _maxRetries;
    SSLCallback _sslCallback;

    RefPtr<SocketConnector> _connector;

#if PRIME_DIRECTHTTPCONNECTION_ENABLE_TRANSCRIPTS
    RefPtr<Stream> _transcriptStream;
#endif
};
}

#endif
