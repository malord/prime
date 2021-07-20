// Copyright 2000-2021 Mark H. P. Lord

// TODO: parsing of IP6 addresses (inet_pton)

#include "SocketAddressParser.h"
#include "StringUtils.h"
#include <stdlib.h>
#include <string.h>

namespace Prime {

const char* SocketAddressParser::parseDottedIP(const char* string, int* out)
{
    for (int n = 0; n != 4; ++n) {
        if (*string < '0' || *string > '9') {
            return NULL;
        }

        char* endPtr;
        out[n] = (int)strtol(string, &endPtr, 0);
        if (endPtr == string) {
            return NULL;
        }

        if (out[n] < 0 || out[n] > 255) {
            return NULL;
        }

        string = endPtr;

        if (n != 3) {
            if (*string++ != '.') {
                return NULL;
            }
        }
    }

    return string;
}

SocketAddressParser::SocketAddressParser()
{
    _result = ResultInvalid;
    _hostname[0] = 0;
    _port = -1;
    _ip[0] = _ip[1] = _ip[2] = _ip[3] = -1;
}

SocketAddressParser::SocketAddressParser(const char* string)
{
    parse(string);
}

bool SocketAddressParser::setHostName(const char* string, size_t length)
{
    return StringCopy(_hostname, sizeof(_hostname), string, length);
}

void SocketAddressParser::reset()
{
    _result = ResultInvalid;
    _hostname[0] = 0;
    _port = -1;
    _ip[0] = _ip[1] = _ip[2] = _ip[3] = -1;
}

bool SocketAddressParser::parse(const char* string)
{
    reset();

    const char* ptr = parseDottedIP(string, &_ip[0]);
    if (ptr) {
        return parsePort(ptr, ResultDottedIP4);
    }

    ptr = string;
    while (*ptr && *ptr != ':') {
        ++ptr;
    }

    if (ptr == string || !setHostName(string, ptr - string)) {
        return false;
    }

    return parsePort(ptr, ResultHostName);
}

bool SocketAddressParser::parseAsHostName(const char* string, int port)
{
    reset();

    if (!setHostName(string, strlen(string))) {
        return false;
    }

    _result = ResultHostName;
    _port = port;

    return true;
}

bool SocketAddressParser::parsePort(const char* ptr, Result currentResult)
{
    if (!*ptr) {
        _port = -1;
        _result = currentResult;
        return true;
    }

    if (*ptr != ':') {
        return false;
    }

    ++ptr;

    char* endPtr;
    _port = (int)strtol(ptr, &endPtr, 0);
    if (endPtr == ptr || *endPtr) {
        return false;
    }

    if (_port < 0 || _port > 65535) {
        return false;
    }

    _result = currentResult;
    return true;
}

bool SocketAddressParser::configureSocketAddress(SocketAddress& addr, int defaultPort, int socketType,
    int* gotSocketType, int protocol, int* gotProtocol, Log* log) const
{
    PRIME_ASSERT(getResult() != ResultInvalid);

    int port = getPort();
    if (port < 0) {
        port = defaultPort;
    }

    if (getResult() == ResultHostName) {
        return addr.resolve(getHostName(), port, socketType, gotSocketType, protocol, gotProtocol, log);
    }

    // From this point on, we need to set *gotSocketType and *gotProtocol ourselves, but only if we succeed.

    switch (getResult()) {
    case ResultDottedIP4: {
        const int* ipAddress = getIP();
        addr.setIP4(ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3], port);
        break;
    }

    default:
        return false;
    }

    if (gotSocketType) {
        *gotSocketType = socketType;
    }
    if (gotProtocol) {
        *gotProtocol = protocol;
    }

    return true;
}

bool SocketAddressParser::configureSocketAddress(SocketAddress& addr, int defaultPort, Log* log) const
{
    return configureSocketAddress(addr, defaultPort, 0, 0, 0, 0, log);
}

bool SocketAddressParser::createAndConnectSocket(Socket& sok, int defaultPort, int socketType, int protocol,
    const Socket::Options& options, int timeoutMilliseconds,
    Log* log, SocketAddress* addr, int* gotSocketType,
    int* gotProtocol) const
{
    SocketAddress tempAddr;
    if (!addr) {
        addr = &tempAddr;
    }

    if (!configureSocketAddress(*addr, defaultPort, socketType, &socketType, protocol, &protocol, log)) {
        return false;
    }

    if (!sok.createForAddress(*addr, socketType, protocol, log, options)) {
        return false;
    }

    if (!sok.connect(*addr, timeoutMilliseconds, log)) {
        return false;
    }

    if (gotSocketType) {
        *gotSocketType = socketType;
    }
    if (gotProtocol) {
        *gotProtocol = protocol;
    }

    return true;
}
}
