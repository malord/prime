// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SOCKS5SERVER_H
#define PRIME_SOCKS5SERVER_H

#include "ArrayView.h"
#include "SocketAddress.h"
#include "StreamBuffer.h"
#include <string>
#include <vector>

namespace Prime {

/// Supports SOCKS4 as well as SOCKS5 (in which case auth() returns true and leaves the 0x04 byte in the buffer).
class SOCKSServerAuth {
public:
    enum Methods {
        NoAuthRequired = UINT8_C(0),
        GSSAPI = UINT8_C(1),
        UsernamePassword = UINT8_C(2)
    };

    void setCredentials(StringView username, StringView password);

    bool auth(StreamBuffer* buffer, Log* log);

    uint8_t getVersion() const { return _version; }

private:
    bool read(StreamBuffer* buffer, Log* log);

    ArrayView<const uint8_t> getMethods() const { return _methods; }

    bool hasMethod(uint8_t method) const;

    bool accept(uint8_t authMethod, StreamBuffer* buffer, Log* log);

    bool usernamePasswordAuth(StreamBuffer* buffer, Log* log);

    uint8_t _version;
    std::vector<uint8_t> _methods;
    std::string _username;
    std::string _password;
};

/// Supports SOCKS4 and translates the command to a SOCKS5 command.
class SOCKSServerCommand {
public:
    enum Command {
        CommandUnknown,
        CommandConnect,
        CommandBind
    };

    enum {
        maxUserID = 8192u
    };

    enum AddressType {
        AddressTypeIP4 = UINT8_C(1),
        AddressTypeHostName = UINT8_C(3),
        AddressTypeIP6 = UINT8_C(4)
    };

    class Options {
    public:
        Options()
            : _lookupDomains(true)
        {
        }

        Options& setLookupDomains(bool value)
        {
            _lookupDomains = value;
            return *this;
        }
        bool getLookupDomains() const { return _lookupDomains; }

    private:
        bool _lookupDomains;
    };

    SOCKSServerCommand();

    bool read(StreamBuffer* buffer, Log* log, const Options& options = Options());

    uint8_t getVersion() const { return _version; }
    Command getCommand() const { return _command; }

    bool isConnect() const { return _command == CommandConnect; }
    bool isBind() const { return _command == CommandBind; }

    /// If the host name is empty, an address is available from getAddress().
    const std::string& getHostName() const { return _domainName; }

    const SocketAddress& getAddress() const { return _address; }

    /// Get the SOCKS4 userid
    const std::string& getUserID() const { return _userid; }

    bool confirmConnect(const SocketAddress& addr, StreamBuffer* stream, Log* log);
    bool denyConnect(StreamBuffer* stream, Log* log);

    bool confirmBind(const SocketAddress& addr, StreamBuffer* stream, Log* log);
    bool denyBind(StreamBuffer* stream, Log* log);

    bool deny(StreamBuffer* stream, Log* log);

private:
    bool readV4(StreamBuffer* buffer, Log* log, const Options& options);

    bool readV5(StreamBuffer* buffer, Log* log, const Options& options);

    bool lookupDomain(Log* log, const Options& options);

    bool replyV4(uint8_t code, const SocketAddress& addr, StreamBuffer* stream, Log* log);

    bool replyV5(uint8_t code, const SocketAddress& addr, StreamBuffer* stream, Log* log);

    std::string _domainName;
    uint16_t _port;
    SocketAddress _address;
    std::string _userid;
    uint8_t _version;
    Command _command;
};
}

#endif
