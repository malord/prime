// Copyright 2000-2021 Mark H. P. Lord

#include "SMTPConnection.h"
#include "Base64Encoder.h"
#include "NumberParsing.h"
#include "QuotedPrintableEncoder.h"
#include "StringUtils.h"
#include "TextEncoding.h"

namespace Prime {

SMTPConnection::SMTPConnection()
{
}

SMTPConnection::~SMTPConnection()
{
}

bool SMTPConnection::init(Stream* stream, Log* log, Stream* transcriptStream)
{
    _log = log;
    _multiStream.reset();
    _multiStream.setReadMode(MultiStream::ReadModeWrite);
    _multiStream.addStream(stream);
    _multiStream.setReadStream(stream);
    _buffer.init(&_multiStream, 8192);

    _authMode = AuthNone;
    _startTLS = false;

    setTranscriptStream(transcriptStream);

    _welcome.resize(0);

    do {
        if (!readResponseLine()) {
            return false;
        }

        _welcome += _response.line;
    } while (_response.continues);

    return true;
}

void SMTPConnection::setTranscriptStream(Stream* stream)
{
    if (_multiStream.getStreamCount() == 1) {
        if (stream) {
            _multiStream.addStream(stream);
        }

    } else {
        if (stream) {
            _multiStream.setStream(1, stream);
        } else {
            _multiStream.removeStream(1);
        }
    }
}

bool SMTPConnection::ehlo(const char* hostname)
{
    _hostname = hostname;

    _buffer.printf(_log, "EHLO %s\r\n", hostname);

    _authMode = AuthNone;
    _startTLS = false;

    do {
        if (!readResponseLine()) {
            return false;
        }

        if (_response.is2xx()) {
            StringView line = StringViewTrim(_response.line);

            if (ASCIIStartsWithIgnoringCase(line, "auth ")) {
                StringView tokens = line.substr(5);
                while (!tokens.empty()) {
                    StringViewPair pair = StringViewBisect(tokens, ' ');
                    tokens = StringViewLeftTrim(pair.second);
                    if (StringsEqualIgnoringCase(pair.first, "login")) {
                        _authMode = AuthLogin;
                    } else if (StringsEqualIgnoringCase(pair.first, "plain")) {
                        _authMode = AuthPlain;
                    }
                }

            } else if (ASCIIStartsWithIgnoringCase(line, "starttls")) {
                _startTLS = true;
            }
        }

        // Record more capabilities?

    } while (_response.continues);

    return true;
}

bool SMTPConnection::readResponseLine(bool expect300s)
{
    _buffer.flushWrites(_log);

    ptrdiff_t index = _buffer.findFirstOf("\r\n", 2, _log);
    if (index < 0) {
        _log->error(PRIME_LOCALISE("No end of line in SMTP response."));
        return false;
    }

    if (index < 4) {
        _log->error(PRIME_LOCALISE("Malformed SMTP response."));
        return false;
    }

    const char* newPtr;
    if (!ParseInt(StringView(_buffer.getReadPointer(), _buffer.getBytesAvailable()), newPtr, _response.code, 10) || newPtr - _buffer.getReadPointer() != 3) {

        _log->error(PRIME_LOCALISE("Malformed SMTP response."));
        return false;
    }

    _response.continues = *newPtr == '-';

    _response.line.assign(newPtr + 1, _buffer.getReadPointer() + index);

    _buffer.skipBytes(index + 2, _log);

    if (_response.code >= 400 || (!expect300s && _response.code >= 300)) {
        _log->error(PRIME_LOCALISE("SMTP response: %d %s"), _response.code, _response.line.c_str());
        return false;
    }

    return true;
}

bool SMTPConnection::startTLS(const SSLCallback& sslCallback)
{
    _buffer.printf(_log, "STARTTLS\r\n");

    do {
        if (!readResponseLine()) {
            return false;
        }
    } while (_response.continues);

    RefPtr<Stream> newStream = sslCallback(_multiStream.getStream(0), _log);
    if (!newStream) {
        return false;
    }

    _multiStream.setStream(0, newStream);
    _multiStream.setReadStream(newStream);

    return true;
}

bool SMTPConnection::auth(const char* username, const char* password, const char* authid)
{
    if (_authMode == AuthLogin) {
        return authLogin(username, password, authid);
    }

    return authPlain(username, password, authid);
}

bool SMTPConnection::authLogin(const char* username, const char* password, const char* authid)
{
    // C: auth login
    // S: 334 VXNlcm5hbWU6 // (base64 of Username)
    // C: avlsdkfj
    // S: 334 UGFzc3dvcmQ6 // (base64 of Password)
    // C: lkajsdfvlj
    // S: 535 authentication failed (#5.7.1)
    //
    // However, there exists a different, RFC compliant version of this behavior, where the client initially
    // sends the userid already with the AUTH LOGIN method:
    //
    // C: AUTH LOGIN ZHVtbXk=
    // S: 334 UGFzc3dvcmQ6
    // C: Z2VoZWlt

    (void)authid;

    _buffer.printf(_log, "AUTH LOGIN\r\n");

    enum { SentUsername = 1u,
        SentPassword = 2u };
    unsigned int sent = 0;

    for (;;) {
        std::string token;

        bool done = false;
        bool success = false;

        do {
            if (!readResponseLine(true)) {
                return false;
            }

            if (_response.code == 334) {
                token = Base64Decode(_response.line);
            } else if (_response.is2xx()) {
                done = true;
                success = true;
            }
        } while (_response.continues);

        if (done) {
            return success;
        }

        StringView send;
        if (ASCIIEqualIgnoringCase(token, "Username:")) {
            sent |= SentUsername;
            send = username;
        } else if (ASCIIEqualIgnoringCase(token, "Password:")) {
            sent |= SentPassword;
            send = password;
        } else if (!(sent & SentUsername)) {
            sent |= SentUsername;
            send = username;
        } else if (!(sent & SentPassword)) {
            sent |= SentPassword;
            send = password;
        } else {
            _log->error(PRIME_LOCALISE("Unknown AUTH LOGIN request."));
            return false;
        }

        _buffer.printf(_log, "%s\r\n", Base64Encode(send).c_str());
    }
}

bool SMTPConnection::authPlain(const char* username, const char* password, const char* authid)
{
    std::string token;
    if (authid) {
        token += authid;
    }
    token += '\0';
    token += username;
    token += '\0';
    token += password;

    std::string base64;
    base64.resize(Base64ComputeMaxEncodedSize(token.size(), 0, 0));
    Base64Encode(&base64[0], base64.size(), token.data(), token.size());

    _buffer.printf(_log, "AUTH PLAIN %s\r\n", base64.c_str());

    do {
        if (!readResponseLine()) {
            return false;
        }
    } while (_response.continues);

    return true;
}

bool SMTPConnection::readResponse(bool expect300s)
{
    do {
        if (!readResponseLine(expect300s)) {
            return false;
        }
    } while (_response.continues);

    return true;
}

bool SMTPConnection::mail(const char* from)
{
    _buffer.printf(_log, "MAIL FROM:<%s>\r\n", from);

    return readResponse();
}

bool SMTPConnection::rcpt(const char* to)
{
    _buffer.printf(_log, "RCPT TO:<%s>\r\n", to);

    return readResponse();
}

bool SMTPConnection::header(const char* name, const char* value)
{
    return _buffer.printf(_log, "%s: %s\r\n", name, value);
}

bool SMTPConnection::headerFormat(const char* name, const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    FormatBufferVA<> buffer(format, argptr);
    va_end(argptr);

    return header(name, buffer.c_str());
}

bool SMTPConnection::line(const char* line)
{
    return _buffer.printf(_log, "%s\r\n", line);
}

bool SMTPConnection::lineFormat(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    FormatBufferVA<> buffer(format, argptr);
    va_end(argptr);

    return line(buffer.c_str());
}

bool SMTPConnection::begin()
{
    _buffer.printf(_log, "DATA\r\n");

    return readResponse(true);
}

bool SMTPConnection::end()
{
    _buffer.printf(_log, ".\r\n");

    do {
        if (!readResponseLine()) {
            return false;
        }
    } while (_response.continues);

    PRIME_ASSERT(_buffer.getBytesAvailable() == 0);

    return true;
}

bool SMTPConnection::quit()
{
    return _buffer.printf(_log, "QUIT\r\n");
}

bool SMTPConnection::quotedPrintable(const void* data, size_t size)
{
    QuotedPrintableEncoder encoder(&_buffer, QuotedPrintableEncoder::Options(76));
    return encoder.writeExact(data, size, _log) && encoder.flush(_log);
}

bool SMTPConnection::quotedPrintable(Stream* stream)
{
    QuotedPrintableEncoder encoder(&_buffer, QuotedPrintableEncoder::Options(76));
    return encoder.copyFrom(stream, _log, -1, _log) && encoder.flush(_log);
}

bool SMTPConnection::base64(const void* data, size_t size)
{
    Base64Encoder encoder(&_buffer, Base64Encoder::Options(76));
    return encoder.writeExact(data, size, _log) && encoder.flush(_log);
}

bool SMTPConnection::base64(Stream* stream)
{
    Base64Encoder encoder(&_buffer, Base64Encoder::Options(76));
    return encoder.copyFrom(stream, _log, -1, _log) && encoder.flush(_log);
}
}
