// Copyright 2000-2021 Mark H. P. Lord

#include "SOCKS5Server.h"
#include "ByteOrder.h"
#include "StringUtils.h"

namespace Prime {

//
// SOCKSServerAuth
//

void SOCKSServerAuth::setCredentials(StringView username, StringView password)
{
    StringCopy(_username, username);
    StringCopy(_password, password);
}

bool SOCKSServerAuth::accept(uint8_t authMethod, StreamBuffer* stream, Log* log)
{
    return stream->writeByte(_version, log) && stream->writeByte(authMethod, log) && stream->flush(log);
}

bool SOCKSServerAuth::hasMethod(uint8_t method) const
{
    return std::find(_methods.begin(), _methods.end(), method) != _methods.end();
}

bool SOCKSServerAuth::read(StreamBuffer* stream, Log* log)
{
    if (!stream->readByte(_version, log)) {
        return false;
    }

    if (_version == 4) {
        stream->putBack(1);
        return true;
    }

    if (_version != 5) {
        log->error(PRIME_LOCALISE("Only v4 and v5 SOCKS protocols are supported."));
        return false;
    }

    uint8_t nmethods;
    if (!stream->readByte(nmethods, log)) {
        return false;
    }

    if (nmethods == 0) {
        log->error(PRIME_LOCALISE("No authentication methods provided."));
        return false;
    }

    _methods.resize(nmethods);
    if (!stream->readBytes(&_methods[0], nmethods, log)) {
        return false;
    }

    return true;
}

bool SOCKSServerAuth::auth(StreamBuffer* stream, Log* log)
{
    if (!read(stream, log)) {
        return false;
    }

    if (_version == 4) {
        return true;
    }

    PRIME_DEBUG_ASSERT(_version == 5);

    if (hasMethod(NoAuthRequired) && _username.empty() && _password.empty()) {
        if (!accept(NoAuthRequired, stream, log)) {
            return false;
        }

        return true;
    }

    if (hasMethod(UsernamePassword)) {
        if (!accept(UsernamePassword, stream, log)) {
            return false;
        }

        return usernamePasswordAuth(stream, log);
    }

    // NOTE: no support for GSSAPI authentication

    accept(0xff, stream, log);
    return false;
}

bool SOCKSServerAuth::usernamePasswordAuth(StreamBuffer* stream, Log* log)
{
    if (stream->readByte(log) != 1) {
        return false;
    }

    uint8_t ulen;
    if (!stream->readByte(ulen, log)) {
        return false;
    }

    std::string uname(ulen, 0);
    if (!stream->readBytes(&uname[0], ulen, log)) {
        return false;
    }

    uint8_t plen;
    if (!stream->readByte(plen, log)) {
        return false;
    }

    std::string passwd(plen, 0);
    if (!stream->readBytes(&passwd[0], plen, log)) {
        return false;
    }

    if (!stream->writeByte(1, log)) {
        return false;
    }

    uint8_t result = (_username == uname && _password == passwd) ? 0 : 0xff;
    if (!stream->writeByte(result, log) || !stream->flush(log)) {
        return false;
    }

    return result == 0;
}

//
// SOCKSServerCommand
//

SOCKSServerCommand::SOCKSServerCommand()
    : _version(0)
    , _command(CommandUnknown)
{
}

bool SOCKSServerCommand::read(StreamBuffer* stream, Log* log, const Options& options)
{
    if (!stream->readByte(_version, log)) {
        return false;
    }

    if (_version == 4) {
        return readV4(stream, log, options);
    }

    if (_version == 5) {
        return readV5(stream, log, options);
    }

    log->error(PRIME_LOCALISE("Unsupported SOCKS version %u."), static_cast<unsigned int>(_version));
    return false;
}

bool SOCKSServerCommand::readV4(StreamBuffer* stream, Log* log, const Options& options)
{
    // The 0x04 has already been read

    uint8_t command;
    if (!stream->readByte(command, log)) {
        return false;
    }

    if (command == 1) {
        _command = CommandConnect;
    } else if (command == 2) {
        _command = CommandBind;
    } else {
        log->error(PRIME_LOCALISE("Unknown SOCKS4 command."));
        return false;
    }

    char buffer[6];
    if (!stream->readBytes(buffer, 6, log)) {
        return false;
    }

    _port = Read16BE(buffer + 0);
    uint32_t dstip = Read32BE(buffer + 2);

    if (!stream->readNullTerminated(_userid, log, maxUserID)) {
        return false;
    }

    if ((dstip & UINT32_C(0xffffff00)) == 0) {
        if (!stream->readNullTerminated(_domainName, log)) {
            return false;
        }

        if (!lookupDomain(log, options)) {
            return false;
        }

    } else {
        _address.setIP4(dstip, _port);
    }

    return true;
}

bool SOCKSServerCommand::readV5(StreamBuffer* stream, Log* log, const Options& options)
{
    // The 0x05 has already been read

    uint8_t buffer[18];
    if (!stream->readBytes(buffer, 3, log)) {
        return false;
    }

    if (buffer[0] == 1) {
        _command = CommandConnect;

    } else if (buffer[0] == 2) {
        _command = CommandBind;

    } else if (buffer[0] == 3) {
        log->error(PRIME_LOCALISE("UDP not supported."));
        return false;

    } else {
        log->error(PRIME_LOCALISE("Unknown SOCKS5 command %u."), static_cast<unsigned int>(buffer[0]));
        return false;
    }

    uint8_t addressType = buffer[2];
    if (addressType == AddressTypeIP4) {
        if (!stream->readBytes(buffer, 6, log)) {
            return false;
        }

        _address.setIP4(Read32BE(buffer), Read16BE(buffer + 4));

    } else if (addressType == AddressTypeHostName) {
        uint8_t len;
        if (!stream->readByte(len, log)) {
            return false;
        }

        _domainName.resize(len);
        if (!stream->readBytes(&_domainName[0], len, log)) {
            return false;
        }

        if (!stream->readBytes(buffer, 2, log)) {
            return false;
        }

        _port = Read16BE(buffer);

        if (!lookupDomain(log, options)) {
            return false;
        }

    } else if (addressType == AddressTypeIP6) {
#ifdef PRIME_NO_IP6
        log->error(PRIME_LOCALISE("IP6 support not present in this build."));
        return false;
#else
        if (!stream->readBytes(buffer, 18, log)) {
            return false;
        }

        _address.setIP6(buffer, Read16BE(buffer + 16));
#endif

    } else {
        log->error(PRIME_LOCALISE("Unknown SOCKS5 address type %u."), static_cast<unsigned int>(addressType));
        return false;
    }

    return true;
}

bool SOCKSServerCommand::lookupDomain(Log* log, const Options& options)
{
    if (!options.getLookupDomains()) {
        return true;
    }

    if (!_address.resolve(_domainName.c_str(), _port, log)) {
        log->error(PRIME_LOCALISE("Cannot resolve host: %s"), _domainName.c_str());
        return false;
    }

    _domainName.resize(0);
    return true;
}

bool SOCKSServerCommand::confirmConnect(const SocketAddress& addr, StreamBuffer* stream, Log* log)
{
    PRIME_ASSERT(_command == CommandConnect);

    if (_version == 4) {
        return replyV4(0x5a, addr, stream, log);
    }

    if (!PRIME_GUARD(_version == 5)) {
        return false;
    }

    return replyV5(0x00, addr, stream, log);
}

bool SOCKSServerCommand::denyConnect(StreamBuffer* stream, Log* log)
{
    PRIME_ASSERT(_command == CommandConnect);

    if (_version == 4) {
        return replyV4(0x5b, SocketAddress(SocketAddress::ip4Any, 0), stream, log);
    }

    if (!PRIME_GUARD(_version == 5)) {
        return false;
    }

    return replyV5(0x01, SocketAddress(SocketAddress::ip4Any, 0), stream, log);
}

bool SOCKSServerCommand::confirmBind(const SocketAddress& addr, StreamBuffer* stream, Log* log)
{
    PRIME_ASSERT(_command == CommandBind);

    if (_version == 4) {
        return replyV4(0x5a, addr, stream, log);
    }

    if (!PRIME_GUARD(_version == 5)) {
        return false;
    }

    return replyV5(0x00, addr, stream, log);
}

bool SOCKSServerCommand::denyBind(StreamBuffer* stream, Log* log)
{
    PRIME_ASSERT(_command == CommandBind);

    if (_version == 4) {
        return replyV4(0x5b, SocketAddress(SocketAddress::ip4Any, 0), stream, log);
    }

    if (!PRIME_GUARD(_version == 5)) {
        return false;
    }

    return replyV5(0x01, SocketAddress(SocketAddress::ip4Any, 0), stream, log);
}

bool SOCKSServerCommand::replyV4(uint8_t code, const SocketAddress& addr, StreamBuffer* stream, Log* log)
{
    if (!addr.isIP4()) {
        log->error(PRIME_LOCALISE("SOCKS4 cannot connect to non-IP4 address."));
        return false;
    }

    uint8_t buffer[8];
    buffer[0] = 0;
    buffer[1] = code;
    Write16BE(buffer + 2, Narrow<uint16_t>(addr.getPort()));
    Write32BE(buffer + 4, addr.getIP4Address());

    return stream->write(buffer, sizeof(buffer), log) && stream->flush(log);
}

bool SOCKSServerCommand::replyV5(uint8_t code, const SocketAddress& addr, StreamBuffer* stream, Log* log)
{
    uint8_t buffer[22];
    buffer[0] = 5;
    buffer[1] = code;
    buffer[2] = 0;

    if (addr.isIP4()) {
        buffer[3] = AddressTypeIP4;
        Write32BE(buffer + 4, addr.getIP4Address());
        Write16BE(buffer + 8, Narrow<uint16_t>(addr.getIP4Port()));

        if (!stream->writeBytes(buffer, 10, log)) {
            return false;
        }

#ifndef PRIME_NO_IP6
    } else if (addr.isIP6()) {
        buffer[3] = AddressTypeIP6;
        memcpy(buffer + 4, addr.getIP6Address(), 16);
        Write16BE(buffer + 20, Narrow<uint16_t>(addr.getIP6Port()));

        if (!stream->writeBytes(buffer, 22, log)) {
            return false;
        }

#endif
    } else {
        log->error(PRIME_LOCALISE("Only IP4 and IP6 addresses are supported by SOCKS5."));
        return false;
    }

    return stream->flush(log);
}

bool SOCKSServerCommand::deny(StreamBuffer* stream, Log* log)
{
    switch (_command) {
    case CommandConnect:
        return denyConnect(stream, log);

    case CommandBind:
        return denyBind(stream, log);

    case CommandUnknown:
        break;
    }

    PRIME_UNREACHABLE();
    return false;
}
}
