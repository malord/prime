// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SMTPCONNECTION_H
#define PRIME_SMTPCONNECTION_H

#include "Config.h"
#include "StreamBuffer.h"
#ifndef PRIME_CXX11_STL
#include "Callback.h"
#endif
#include "MultiStream.h"
#include <functional>

namespace Prime {

/// Send mail via SMTP. Supports TLS via a callback which you must provide.
class PRIME_PUBLIC SMTPConnection {
public:
    SMTPConnection();

    ~SMTPConnection();

    bool init(Stream* stream, Log* log, Stream* transcriptStream = NULL);

    void setTranscriptStream(Stream* stream);

    bool ehlo(const char* hostname);

#ifdef PRIME_CXX11_STL
    typedef std::function<RefPtr<Stream>(Stream*, Log*)> SSLCallback;
#else
    typedef Callback2<RefPtr<Stream>, Stream*, Log*> SSLCallback;
#endif

    /// Returns true if the EHLO reported the STARTTLS capability.
    bool getStartTLS() const { return _startTLS; }

    /// Invokes the callback to wrap the connection Stream.
    bool startTLS(const SSLCallback& sslCallback);

    enum AuthMode {
        AuthNone,
        AuthPlain,
        AuthLogin
    };

    /// ehlo() will set this for you if the server responds with an authentication mechanism.
    void setAuthMode(AuthMode mode) { _authMode = mode; }

    AuthMode getAuthMode() const { return _authMode; }

    /// Uses getAuthMode() to figure out which auth method to call.
    bool auth(const char* username, const char* password, const char* authid = NULL);

    bool authPlain(const char* username, const char* password, const char* authid);

    bool authLogin(const char* username, const char* password, const char* authid);

    bool mail(const char* from);

    bool rcpt(const char* to);

    bool header(const char* name, const char* value);

    bool headerFormat(const char* name, const char* format, ...);

    bool line(const char* line);

    bool lineFormat(const char* format, ...);

    bool begin();

    bool end();

    bool readResponse(bool expect300s = false);

    bool quotedPrintable(const void* data, size_t size);

    bool quotedPrintable(Stream* sourceStream);

    bool base64(const void* data, size_t size);

    bool base64(Stream* sourceStream);

    bool quit();

private:
    bool readResponseLine(bool expect300s = false);

    MultiStream _multiStream;
    StreamBuffer _buffer;
    RefPtr<Log> _log;

    std::string _hostname;

    std::string _welcome;

    struct Response {
        std::string line;
        int code;
        bool continues;

        bool is2xx() const { return code >= 200 && code <= 299; }
    } _response;

    AuthMode _authMode;
    bool _startTLS;
};
}

#endif
