// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SOCKETADDRESS_H
#define PRIME_SOCKETADDRESS_H

#include "SocketSupport.h"
#include <string>
#include <vector>

namespace Prime {

/// Encapsulates a socket address (i.e., struct sockaddr).
class PRIME_PUBLIC SocketAddress {
public:
    /// IP4 "any" IP address.
    PRIME_STATIC_CONST(uint32_t, ip4Any, 0);

    /// IP4 broadcast address.
    PRIME_STATIC_CONST(uint32_t, ip4Broadcast, 0xffffffff);

    /// IP4 loopback address.
    PRIME_STATIC_CONST(uint32_t, ip4Localhost, 0x7f000001);

    /// IP4 "no" address.
    PRIME_STATIC_CONST(uint32_t, ip4None, 0xffffffff);

    /// Pack 4 numbers in to a 32-bit IP4 address.
    static uint32_t packIP4(int a, int b, int c, int d)
    {
        return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | ((uint32_t)d);
    }

    /// Unpack an IP4 address in to 4 numbers.
    static void unpackIP4(uint32_t addr, int numbers[4]);

    /// Copies at most bufferSize - 1 bytes in to buffer and null terminates it. On error, returns false and
    /// writes an error message to the log. If the buffer is small, true is returned and *requiredSize will
    /// be >= bufferSize.
    static bool getHostName(char* buffer, size_t bufferSize, Log* log, size_t* requiredSize = NULL);

    struct AddressInfo;

    /// Resolve a host name. Dotted IP addresses may not be supported. This variant of resolve() allows the
    /// desired socket type and protocol to be specified, with the actual socket type and protocol returned in
    /// *gotSocketType and *gotProtocol. Set socketType to 0 to let the implementation decide or to, e.g.,
    /// SOCK_STREAM. Set protocol to 0 to let the implementation decide, or, e.g., IPPROTO_TCP.
    static bool resolve(std::vector<AddressInfo>& addresses, const char* hostname, int port, int socketType,
        int protocol, Log* log);

#ifdef PRIME_OS_HAS_GETIFADDRS
    static bool getAllInterfaceAddresses(std::vector<SocketAddress>& addresses, Log* log);
#endif

    /// Returns an array of strings containing all the local addresses, e.g., "192.168.0.1:8080". If port is
    /// zero, it is not appended to the output strings.
    static bool getAllInterfaceAddresses(std::vector<std::string>& address, int port, Log* log);

    /// Initialise this object to null (isNull() will return true).
    SocketAddress();

    /// Copy constructor.
    SocketAddress(const SocketAddress& copy);

    /// Initialise with an IP4 address.
    explicit SocketAddress(const sockaddr_in& addr);

    /// Initialise an IP4 address.
    SocketAddress(int a, int b, int c, int d, int port);

    /// Initialise an IP4 address.
    SocketAddress(uint32_t ip, int port);

    /// Initialise a socket address with bytes.
    SocketAddress(const void* bytes, size_t byteCount);

    ~SocketAddress()
    {
        if (_data != &_builtin) {
            delete[](char*) _data;
        }
    }

    /// If this is a null SocketAddress, returns true.
    bool isNull() const { return _length == 0; }

    /// Unary not operator test for nullness.
    bool operator!() const { return _length == 0; }

    /// Get the length of the address.
    SocketSupport::AddressLength getLength() const { return (SocketSupport::AddressLength)_length; }

    /// Set the length of the address. This ensures enough bytes are allocated.
    void setLength(size_t newLength);

    /// Set the address.
    void set(const void* bytes, size_t byteCount);

    /// Get the address. It is writable.
    sockaddr* get() { return &_data->addr; }

    /// Get the address. It is writable.
    const sockaddr* get() const { return &_data->addr; }

    /// Get as a sockaddr_in. Does not verify the address format.
    sockaddr_in* getSockaddrIN() { return &_data->in; }

    /// Get as a sockaddr_in. Does not verify the address format.
    const sockaddr_in* getSockaddrIN() const { return &_data->in; }

    /// Get the address family.
    int getFamily() const
    {
        if (_length < sizeof(sockaddr)) {
            return -1;
        }

        return _data->addr.sa_family;
    }

    /// Returns true if this is an IP4 address.
    bool isIP4() const { return _length == sizeof(sockaddr_in) && _data->in.sin_family == AF_INET; }

    /// Get the IP address part of an IP4 address.
    uint32_t getIP4Address() const { return ntohl(_data->in.sin_addr.s_addr); }

    /// Retrieve the IP address part of an IP4 address as an arry of numbers.
    void getIP4Address(int array[4]) const;

    /// Get the port part of an IP4 address.
    int getIP4Port() const { return (int)ntohs(_data->in.sin_port); }

    /// Initialise with an IP4 address.
    void setIP4(const sockaddr_in& addr) { set(&addr, sizeof(addr)); }

    /// Initialise an IP4 address.
    void setIP4(int a, int b, int c, int d, int port);

    /// Initialise an IP4 address.
    void setIP4(uint32_t ip, int port);

    /// Set the IP4 port.
    void setIP4Port(int port);

    bool isIP4Localhost() const { return isIP4() && getIP4Address() == ip4Localhost; }

    /// Resolve a host name. Dotted IP addresses are not supported.
    bool resolve(const char* hostname, int port, Log* log);

    /// Resolve a host name. Dotted IP addresses may not be supported. This variant of resolve() allows the
    /// desired socket type and protocol to be specified, with the actual socket type and protocol returned in
    /// *gotSocketType and *gotProtocol. Set socketType to 0 to let the implementation decide or to, e.g.,
    /// SOCK_STREAM. Set protocol to 0 to let the implementation decide, or, e.g., IPPROTO_TCP.
    bool resolve(const char* hostname, int port, int socketType, int* gotSocketType, int protocol, int* gotProtocol, Log* log);

    bool describe(char* buffer, size_t bufferSize, bool withPort = false) const;

    std::string getDescription() const;

    std::string getDescriptionWithPort() const;

    /// Differs from getDescriptionWithPort() in that the return is guaranteed to be usable in a browser.
    std::string toString() const;

    /// Assignment operator.
    SocketAddress& operator=(const SocketAddress& copy);

    /// Comparison operator.
    bool operator==(const SocketAddress& other) const;

    /// Inequality operator.
    bool operator!=(const SocketAddress& other) const { return !operator==(other); }

#ifndef PRIME_NO_IP6

    /// Returns true if this is an IP6 address.
    bool isIP6() const { return _length == sizeof(sockaddr_in6) && _data->in.sin_family == AF_INET6; }

    /// Get the port part of an IP6 address.
    int getIP6Port() const
    {
        PRIME_DEBUG_ASSERT(isIP6());
        return (int)ntohs(_data->in6.sin6_port);
    }

    /// Get the address octets of an IP6 address.
    const void* getIP6Address() const
    {
        PRIME_DEBUG_ASSERT(isIP6());
        return _data->in6.sin6_addr.s6_addr;
    }

    /// Set the IP6 address and port.
    void setIP6(const void* bytes, uint16_t port, uint16_t scopeID = 0);

    /// Set the IP6 port.
    void setIP6Port(int port);

    /// Reverse lookup the name of this address.
    bool getNameInfo(char* host, size_t hostSize, char* service, size_t serviceSize, int flags, Log* log) const;

#endif

    /// Returns true if the address is IANDDR_ANY or in6addr_any.
    bool isAny() const;

    /// Returns true if the address is localhost.
    bool isLocalhost() const;

    /// Returns the port number for all address types which support it.
    int getPort() const;

    /// Set the port number for any address type which supports it.
    void setPort(int port);

private:
    /// Initialise to null.
    void constructNull()
    {
        _data = &_builtin;
        _length = 0;
    }

    /// Initialise with the specified address length.
    void constructLength(size_t len);

    std::string getDescription(bool withPort) const;

    union AddrStore {
        sockaddr addr;
        sockaddr_in in;
#ifndef PRIME_NO_IP6
        sockaddr_in6 in6;
        char raw[sizeof(sockaddr_storage)];
#else
        char raw[28]; // Room for a sockaddr_in6.
#endif
    };

    // More often than not, no memory will be allocated and builtin will be used.
    AddrStore _builtin;
    AddrStore* _data;
    size_t _length;

public:
    /// The maximum size of a socket address of the known formats. A socket address can exceed this size but will
    /// incur dynamic memory allocation.
    enum { maxAddrSize = sizeof(AddrStore) };
};

struct SocketAddress::AddressInfo {
    SocketAddress address;
    int socketType;
    int protocol;
};
}

#endif
