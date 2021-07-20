// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SOCKETADDRESSPARSER_H
#define PRIME_SOCKETADDRESSPARSER_H

#include "Socket.h"
#include "SocketAddress.h"

namespace Prime {

/// Parses a string provided by the user to identify a host and, optionally, a port.
class PRIME_PUBLIC SocketAddressParser {
public:
    /// Try to parse the specified string as a dotted IP address. Returns a null pointer on error, otherwise
    /// returns a pointer to the first character after the IP address (which should be a \\0 or a :).
    static const char* parseDottedIP(const char* string, int* out);

    SocketAddressParser();

    /// Parse the specified string. Whitespace is significant.
    explicit SocketAddressParser(const char* string);

    /// Parse the specified string. Whitespace is significant.
    bool parse(const char* string);

    /// Set a host name with the specified port. This can be used to support IP6 addresses.
    bool parseAsHostName(const char* string, int port = -1);

    void reset();

    /// Result of a Parse operation.
    enum Result {
        /// The string is incomprehensible.
        ResultInvalid,

        /// It's a host name.
        ResultHostName,

        /// It's a dotted IP address.
        ResultDottedIP4
    };

    /// What was parsed?
    Result getResult() const { return _result; }

    /// Return the host name.
    const char* getHostName() const { return _hostname; }

    /// Return the dotted IP address.
    const int* getIP() const { return _ip; }

    /// Get the port number that was parsed. Will be -1 if no port was parsed or the port was invalid.
    int getPort() const { return _port; }

    /// Get the port number, returning defaultPort if no port was parsed.
    int getPort(int defaultPort) const { return _port < 0 ? defaultPort : _port; }

    /// Configure a SocketAddress with our result.
    bool configureSocketAddress(SocketAddress& addr, int defaultPort, Log* log) const;

    /// Configures a SocketAddress, supplying the desired socket type and protocol to allow getaddrinfo() to
    /// be used for IP6 support.
    bool configureSocketAddress(SocketAddress& addr, int defaultPort, int socketType, int* gotSocketType,
        int protocol, int* gotProtocol, Log* log) const;

    /// Calls create() and connect() on a socket with the correct address.
    bool createAndConnectSocket(Socket& sok, int defaultPort, int socketType, int protocol,
        const Socket::Options& options, int timeoutMillisecods, Log* log,
        SocketAddress* addr = NULL, int* gotSocketType = NULL,
        int* gotProtocol = NULL) const;

private:
    /// Finish off a Parse by checking the specified pointer to see if it's a port number.
    bool parsePort(const char* ptr, Result currentResult);

    bool setHostName(const char* string, size_t length);

    Result _result;
    char _hostname[SocketSupport::maxHostName];
    int _ip[4];
    int _port;
};

}

#endif
