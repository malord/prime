// Copyright 2000-2021 Mark H. P. Lord

// This code assumes address families (AF_INET) are equal to protocol families (PF_INET).

#include "SocketAddress.h"
#include "StringUtils.h"
#include "Templates.h"
#include <string.h>

namespace Prime {

void SocketAddress::unpackIP4(uint32_t addr, int numbers[4])
{
    numbers[0] = (int)((addr >> 24) & 0xff);
    numbers[1] = (int)((addr >> 16) & 0xff);
    numbers[2] = (int)((addr >> 8) & 0xff);
    numbers[3] = (int)((addr)&0xff);
}

bool SocketAddress::getHostName(char* buffer, size_t bufferSize, Log* log, size_t* requiredSize)
{
    PRIME_ASSERT(bufferSize != 0);

    *buffer = 0;

    char name[256];

    int err = gethostname(name, sizeof(name));
    if (err != 0) {
        log->logErrno(errno);
        return false;
    }

    if (requiredSize) {
        *requiredSize = strlen(name);
    }

    StringCopy(buffer, bufferSize, name);

    return true;
}

SocketAddress::SocketAddress()
{
    constructNull();
}

SocketAddress::SocketAddress(const SocketAddress& copy)
{
    constructNull();
    operator=(copy);
}

SocketAddress::SocketAddress(const sockaddr_in& addr)
{
    _data = &_builtin;
    _data->in = addr;
}

SocketAddress::SocketAddress(int a, int b, int c, int d, int port)
{
    _data = &_builtin;
    _length = sizeof(_data->in);
    memset(&_data->in, 0, sizeof(_data->in));
    //_data->in.sin_len = sizeof(sockaddr_in);
    _data->in.sin_family = AF_INET;
    _data->in.sin_addr.s_addr = htonl(packIP4(a, b, c, d));
    _data->in.sin_port = htons((unsigned short)port);
}

SocketAddress::SocketAddress(uint32_t ip, int port)
{
    _data = &_builtin;
    _length = sizeof(_data->in);
    memset(&_data->in, 0, sizeof(_data->in));
    //_data->in.sin_len = sizeof(sockaddr_in);
    _data->in.sin_family = AF_INET;
    _data->in.sin_addr.s_addr = htonl(ip);
    _data->in.sin_port = htons((unsigned short)port);
}

SocketAddress::SocketAddress(const void* bytes, size_t byteCount)
{
    constructLength(byteCount);
    memcpy(_data->raw, bytes, byteCount);
}

void SocketAddress::constructLength(size_t len)
{
    if (len > sizeof(AddrStore)) {
        _data = (AddrStore*)new char[len];
    } else {
        _data = &_builtin;
    }

    _length = len;
}

void SocketAddress::setLength(size_t newLength)
{
    if (_data != &_builtin) {
        delete[](char*) _data;
    }

    constructLength(newLength);
}

void SocketAddress::set(const void* bytes, size_t byteCount)
{
    setLength(byteCount);
    memcpy(_data->raw, bytes, byteCount);
}

void SocketAddress::getIP4Address(int array[4]) const
{
    unpackIP4(getIP4Address(), array);
}

SocketAddress& SocketAddress::operator=(const SocketAddress& copy)
{
    if (this == &copy) {
        return *this;
    }

    if (_data != &_builtin) {
        delete[](char*) _data;
    }

    constructLength(copy._length);
    memcpy(_data->raw, copy._data->raw, _length);

    return *this;
}

bool SocketAddress::operator==(const SocketAddress& other) const
{
    if (_length != other._length) {
        return false;
    }

    if (!_length) {
        return true; // both null.
    }

    // A byte-for-byte comparison doesn't always work due to fields in sockaddr that aren't in other
    // implementations. For example, some BSDs (and OS X) have a sin_len field in sockaddr_in which is not
    // present in Linux or Windows. So, we have to do special case comparisons.
    if (isIP4()) {
        return other.isIP4() && _data->in.sin_port == other._data->in.sin_port && _data->in.sin_addr.s_addr == other._data->in.sin_addr.s_addr;
    }

    return memcmp(_data->raw, other._data->raw, _length) == 0;
}

void SocketAddress::setIP4(int a, int b, int c, int d, int port)
{
    setLength(sizeof(sockaddr_in));
    memset(_data->raw, 0, sizeof(sockaddr_in));
    //_data->in.sin_len = sizeof(sockaddr_in);
    _data->in.sin_family = AF_INET;
    _data->in.sin_addr.s_addr = htonl(packIP4(a, b, c, d));
    _data->in.sin_port = htons((unsigned short)port);
}

void SocketAddress::setIP4(uint32_t ip, int port)
{
    setLength(sizeof(sockaddr_in));
    memset(_data->raw, 0, sizeof(sockaddr_in));
    //_data->in.sin_len = sizeof(sockaddr_in);
    _data->in.sin_family = AF_INET;
    _data->in.sin_addr.s_addr = htonl(ip);
    _data->in.sin_port = htons((unsigned short)port);
}

void SocketAddress::setIP4Port(int port)
{
    PRIME_ASSERT(isIP4());
    _data->in.sin_port = htons((unsigned short)port);
}

bool SocketAddress::resolve(const char* hostname, int port, Log* log)
{
    return resolve(hostname, port, 0, 0, 0, 0, log);
}

bool SocketAddress::resolve(const char* hostname, int port, int socketType, int* gotSocketType, int protocol,
    int* gotProtocol, Log* log)
{
    std::vector<AddressInfo> addresses;
    if (!resolve(addresses, hostname, port, socketType, protocol, log) || addresses.empty()) {
        return false;
    }

    *this = addresses[0].address;
    if (gotSocketType) {
        *gotSocketType = addresses[0].socketType;
    }
    if (gotProtocol) {
        *gotProtocol = addresses[0].protocol;
    }

    return true;
}

bool SocketAddress::getAllInterfaceAddresses(std::vector<std::string>& addresses, int port, Prime::Log* log)
{
    char hostname[256];
    if (SocketAddress::getHostName(hostname, sizeof(hostname), log)) {
        PushBackUnique(addresses, Format("%s:%d", hostname, port));
    }

#ifdef PRIME_OS_HAS_GETIFADDRS
    std::vector<SocketAddress> ifAddresses;
    if (SocketAddress::getAllInterfaceAddresses(ifAddresses, log)) {
        bool anyIP4s = false;
        for (size_t i = 0; i != ifAddresses.size(); ++i) {
            if (ifAddresses[i].isIP4() && (ifAddresses[i].getIP4Address() & UINT32_C(0xff000000)) != UINT32_C(0x7f000000)) {

                anyIP4s = true;
                break;
            }
        }

        for (size_t i = 0; i != ifAddresses.size(); ++i) {
            if (anyIP4s && !ifAddresses[i].isIP4()) {
                continue;
            }

            char buffer[256];
            ifAddresses[i].setPort(port);
            if (ifAddresses[i].describe(buffer, sizeof(buffer), true)) {
                PushBackUnique(addresses, buffer);
            }
        }
    }

#else
    std::vector<SocketAddress::AddressInfo> addressInfos;
    if (SocketAddress::resolve(addressInfos, hostname, port, SOCK_STREAM, IPPROTO_TCP, log)) {
        bool anyIP4s = false;
        for (size_t i = 0; i != addressInfos.size(); ++i) {
            if (addressInfos[i].address.isIP4() && (addressInfos[i].address.getIP4Address() & UINT32_C(0xff000000)) != UINT32_C(0x7f000000)) {

                anyIP4s = true;
                break;
            }
        }
        for (size_t i = 0; i != addressInfos.size(); ++i) {
            if (anyIP4s && !addressInfos[i].address.isIP4()) {
                continue;
            }

            char buffer[256];
            if (addressInfos[i].address.describe(buffer, sizeof(buffer), true)) {
                PushBackUnique(addresses, buffer);
            }
        }
    }
#endif

    return true;
}

#ifdef PRIME_OS_HAS_GETIFADDRS
bool SocketAddress::getAllInterfaceAddresses(std::vector<SocketAddress>& addresses, Log* log)
{
    struct ifaddrs* ifa;
    if (getifaddrs(&ifa) != 0) {
        log->logErrno(errno);
        return false;
    }

    for (struct ifaddrs* ptr = ifa; ptr; ptr = ptr->ifa_next) {
        if (!ptr->ifa_addr) {
            continue;
        }

        SocketAddress addr;

        if (ptr->ifa_addr->sa_family == AF_INET) {
            addr.set(ptr->ifa_addr, sizeof(sockaddr_in));
#ifndef PRIME_NO_IP6
        } else if (ptr->ifa_addr->sa_family == AF_INET6) {
            addr.set(ptr->ifa_addr, sizeof(sockaddr_in6));
#endif
        } else {
            continue;
        }

        addresses.push_back(addr);
    }

    freeifaddrs(ifa);
    return true;
}
#endif

bool SocketAddress::resolve(std::vector<AddressInfo>& addresses, const char* hostname, int port,
    int socketType, int protocol, Log* log)
{
#ifndef PRIME_NO_IP6

    // If you ever need to get IP6 addresses, change < 2 to < 3 and uncomment the conditional below
    for (int family = 0; family < 2; ++family) {

        addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = family == 1 ? PF_INET :
                                      //family == 2 ? PF_INET6 :
            PF_UNSPEC;
        hints.ai_socktype = socketType;
        hints.ai_protocol = protocol;

        char portstr[16];
        StringFormat(portstr, "%d", port);

        addrinfo* result = NULL;
        int retval = getaddrinfo(hostname, portstr, &hints, &result);
        if (retval != 0 || !result) {
            SocketSupport::logGetAddrInfoError(log, retval);
            return false;
        }

        for (const addrinfo* ptr = result; ptr; ptr = ptr->ai_next) {
            addresses.push_back(AddressInfo());
            AddressInfo& ai = addresses.back();

            ai.address.set(ptr->ai_addr, ptr->ai_addrlen);
            ai.socketType = ptr->ai_socktype;
            ai.protocol = ptr->ai_protocol;
        }

        freeaddrinfo(result);
    }

    // Move IPv4 before IPv6

    size_t first6 = SIZE_MAX;
    for (size_t i = 0; i != addresses.size(); ++i) {
        const AddressInfo& ai = addresses[i];

        switch (ai.address.getFamily()) {
        case AF_INET6:
            if (i < first6) {
                first6 = i;
            }
            break;

        case AF_INET:
            if (first6 != SIZE_MAX) {
                AddressInfo copy(ai);
                addresses.erase(addresses.begin() + i);
                addresses.insert(addresses.begin() + first6, copy);
                ++first6;
            }
            break;
        }
    }

    return true;

#else

    for (;;) {
        hostent* he = gethostbyname(hostname);
        if (!he) {
            if (SocketSupport::getLastSocketError() == SocketSupport::ErrorInterrupt) {
                continue;
            }

            SocketSupport::logSocketError(log, SocketSupport::getLastSocketError());
            return false;
        }

        AddressInfo ai;

        switch (he->h_addrtype) {
        case AF_INET:
            ai.address.setIP4(ntohl(((const in_addr*)he->h_addr)->s_addr), port);
            break;

        default:
            return false;
        }

        ai.socketType = socketType;
        ai.protocol = protocol;

        addresses.push_back(ai);

        return true;
    }

#endif
}

std::string SocketAddress::getDescription(bool withPort) const
{
    char buffer[1024];
    if (!describe(buffer, sizeof(buffer), withPort)) {
        return "";
    }

    return buffer;
}

std::string SocketAddress::getDescription() const
{
    return getDescription(false);
}

std::string SocketAddress::getDescriptionWithPort() const
{
    return getDescription(true);
}

std::string SocketAddress::toString() const
{
    return getDescription(true);
}

bool SocketAddress::describe(char* buffer, size_t bufferSize, bool withPort) const
{
#ifndef PRIME_NO_IP6

    if (isIP4()) {
        char p[INET_ADDRSTRLEN + 1];
        inet_ntop(AF_INET, &_data->in.sin_addr, p, sizeof(p) - 1);
        StringCopy(buffer, bufferSize, p);

        if (withPort) {
            StringAppendFormat(buffer, bufferSize, ":%d", (int)getIP4Port());
        }

        return true;
    }

    if (isIP6()) {
        char p[INET6_ADDRSTRLEN + 1];
        inet_ntop(AF_INET6, &_data->in6.sin6_addr, p, sizeof(p) - 1);
        StringCopy(buffer, bufferSize, p);

        if (withPort) {
            StringAppendFormat(buffer, bufferSize, ":%d", (int)getIP6Port());
        }

        return true;
    }

#else

    if (isIP4()) {
        StringCopy(buffer, bufferSize, inet_ntoa(_data->in.sin_addr));

        if (withPort) {
            StringAppendFormat(buffer, bufferSize, ":%d", (int)getIP4Port());
        }
    }

#endif

    DeveloperWarning("SocketAddress: Unknown SocketAddress type.");
    return false;
}

bool SocketAddress::isAny() const
{
    if (isIP4()) {
        return getIP4Address() == 0;
    }

#ifndef PRIME_NO_IP6
    if (isIP6()) {
        return IN6_IS_ADDR_UNSPECIFIED(&_data->in6.sin6_addr) ? true : false;
    }
#endif

    DeveloperWarning("SocketAddress: Unsupported address type.");
    return false;
}

bool SocketAddress::isLocalhost() const
{
    if (isIP4()) {
        return isIP4Localhost();
    }

#ifndef PRIME_NO_IP6
    if (isIP6()) {
        return IN6_IS_ADDR_LINKLOCAL(&_data->in6.sin6_addr) ? true : false;
    }
#endif

    DeveloperWarning("SocketAddress: Unsupported address type.");
    return false;
}

int SocketAddress::getPort() const
{
    if (isIP4()) {
        return getIP4Port();
    }

#ifndef PRIME_NO_IP6
    if (isIP6()) {
        return getIP6Port();
    }
#endif

    DeveloperWarning("SocketAddress: Unsupported address type.");
    return -1;
}

void SocketAddress::setPort(int port)
{
    if (isIP4()) {
        setIP4Port(port);
        return;
    }

#ifndef PRIME_NO_IP6
    if (isIP6()) {
        setIP6Port(port);
        return;
    }
#endif

    DeveloperWarning("SocketAddress: Unsupported address type.");
}

#ifndef PRIME_NO_IP6

void SocketAddress::setIP6(const void* bytes, uint16_t port, uint16_t scopeID)
{
    setLength(sizeof(sockaddr_in6));
    memset(_data->raw, 0, sizeof(sockaddr_in6));
    //_data->in6.sin6_len = sizeof(sockaddr_in6);
    _data->in6.sin6_family = AF_INET6;
    memcpy(_data->in6.sin6_addr.s6_addr, bytes, 16);
    _data->in6.sin6_port = htons((unsigned short)port);
    _data->in6.sin6_scope_id = scopeID;
}

void SocketAddress::setIP6Port(int port)
{
    PRIME_ASSERT(isIP6());
    _data->in6.sin6_port = htons((unsigned short)port);
}

bool SocketAddress::getNameInfo(char* host, size_t hostSize, char* service, size_t serviceSize, int flags, Log* log) const
{
    if (!PRIME_GUARD(hostSize) || !PRIME_GUARD(serviceSize)) {
        return false;
    }

    PRIME_ASSERT(hostSize == (size_t)(SocketSupport::AddressLength)hostSize);
    PRIME_ASSERT(serviceSize == (size_t)(SocketSupport::AddressLength)serviceSize);

    int result = getnameinfo(&_data->addr, (SocketSupport::AddressLength)_length, host, (SocketSupport::AddressLength)hostSize, service, (SocketSupport::AddressLength)serviceSize, flags);

    if (result != 0) {
        SocketSupport::logGetAddrInfoError(log, result);
        return false;
    }

    host[hostSize - 1] = 0;
    service[serviceSize - 1] = 0;

    return true;
}

#endif
}
