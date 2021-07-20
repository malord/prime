// Copyright 2000-2021 Mark H. P. Lord

#include "SOCKS5Stream.h"
#include "SocketAddressParser.h"

namespace Prime {

PRIME_DEFINE_UID_CAST(SOCKS5Stream)

SOCKS5Stream::~SOCKS5Stream()
{
}

bool SOCKS5Stream::init(const SocketAddress& addr, StringView username, StringView password, Log* log,
    SOCKSVersion socksVersion)
{
    if (username.size() > 255 || password.size() > 255) {
        log->error(PRIME_LOCALISE("SOCKS5 username or password exceed maximum size of 255 characters."));
        return false;
    }

    _username.assign(username.begin(), username.end());
    _password.assign(password.begin(), password.end());
    _proxyAddress = addr;
    _socksVersion = socksVersion;
    return true;
}

bool SOCKS5Stream::connect(const char* hostname, int port, Log* log)
{
    PRIME_ASSERT(port > 0 && port < 65546);

    // Take advantage of a known IP address
    int ip4[4];
    if (SocketAddressParser::parseDottedIP(hostname, ip4)) {
        SocketAddress addr;
        addr.setIP4(ip4[0], ip4[1], ip4[2], ip4[3], port);
        return connect(addr, log);
    }

    return initSOCKS5(CommandConnect, NULL, hostname, port, log);
}

bool SOCKS5Stream::connect(const SocketAddress& addr, Log* log)
{
    return initSOCKS5(CommandConnect, &addr, NULL, 0, log);
}

bool SOCKS5Stream::initSOCKS5(Command command, const SocketAddress* addr, const char* hostname, int port, Log* log)
{
    if (addr && addr->isIP4()) {
        log->error(PRIME_LOCALISE("Only IP4 is supported via SOCKS5."));
        return false;
    }

    Socket& socket = this->accessSocket();

    if (!socket.createForAddress(_proxyAddress, SOCK_STREAM, IPPROTO_TCP, log, Socket::Options())) {
        return false;
    }

    if (!socket.connect(_proxyAddress, getReadTimeout(), log)) {
        return false;
    }

    uint8_t buffer[1024];

    uint8_t* bufptr = buffer;
    *bufptr++ = 5;

    if (!_username.empty() || !_password.empty()) {
        *bufptr++ = 2; // We support 2 methods:
        *bufptr++ = 0; // No auth
        *bufptr++ = 2; // Plain text username/password
    } else {
        *bufptr++ = 1;
        *bufptr++ = 0;
    }

    if (!this->writeExact(buffer, static_cast<size_t>(bufptr - buffer), log)) {
        return false;
    }

    if (!this->readExact(buffer, 2, log)) {
        return false;
    }

    if (buffer[1] != 0 && buffer[2] != 0) {
        log->error(PRIME_LOCALISE("SOCKS5 proxy does not provide a compatible authentication mechanism."));
        return false;
    }

    if (buffer[1] == 2) {
        bufptr = buffer;

        if (_username.size() + _password.size() + 3 > sizeof(buffer)) {
            log->error(PRIME_LOCALISE("SOCKS5 username/password too long."));
            return false;
        }

        *bufptr++ = 1;
        *bufptr++ = Narrow<uint8_t>(_username.size());
        if (!_username.empty()) {
            memcpy(bufptr, _username.data(), _username.size());
            bufptr += _username.size();
        }
        *bufptr++ = Narrow<uint8_t>(_password.size());
        if (!_password.empty()) {
            memcpy(bufptr, _password.data(), _password.size());
            bufptr += _password.size();
        }

        if (!this->writeExact(buffer, static_cast<size_t>(bufptr - buffer), log)) {
            return false;
        }

        if (!this->readExact(buffer, 2, log)) {
            return false;
        }

        if (buffer[1] != 0) {
            log->error(PRIME_LOCALISE("SOCKS5 login failed."));
            return false;
        }
    }

    bufptr = buffer;
    *bufptr++ = 5;
    *bufptr++ = Narrow<uint8_t>(command);
    *bufptr++ = 0;

    if (addr) {
        *bufptr++ = 1;
        PRIME_WRITE32BE(bufptr, addr->getIP4Address());
        bufptr += 4;
    } else {
        *bufptr++ = 3;
        size_t nameLength = strlen(hostname);
        if (nameLength + 3 > sizeof(buffer) - static_cast<size_t>(bufptr - buffer) || nameLength > 255) {
            log->error(PRIME_LOCALISE("SOCKS5 hostname too large."));
            return false;
        }

        *bufptr++ = Narrow<uint8_t>(nameLength);
        memcpy(bufptr, hostname, nameLength);
        bufptr += nameLength;
    }

    if (!addr) {
        PRIME_WRITE16BE(bufptr, Narrow<uint16_t>(port));
    } else {
        PRIME_WRITE16BE(bufptr, addr->getIP4Port());
    }
    bufptr += 2;

    if (!this->writeExact(buffer, static_cast<size_t>(bufptr - buffer), log)) {
        return false;
    }
    if (!this->readExact(buffer, 4, log)) {
        return false;
    }

    if (buffer[1] != 0) {
        // TODO: could look for the IP address ourself
        log->error(PRIME_LOCALISE("SOCKS5 could not connect to host."));
        return false;
    }

    if (buffer[3] == 1) {
        // Responded with IP4 address.
        if (!this->readExact(&buffer[4], 6, log)) {
            return false;
        }
    } else if (buffer[3] == 3) {
        if (!this->readExact(&buffer[4], 1, log)) {
            return false;
        }

        uint8_t nameLength = buffer[4];
        if (!this->readExact(&buffer[5], nameLength + 2u, log)) {
            return false;
        }
    } else {
        log->error(PRIME_LOCALISE("SOCKS5 responded with unsupported address type."));
        return false;
    }

    return true;
}

}
