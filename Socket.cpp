// Copyright 2000-2021 Mark H. P. Lord

#include "Socket.h"
#include "NumberUtils.h"
#include "SignalSocket.h"
#include <string.h>

namespace Prime {

bool Socket::handleError(Log* log)
{
    return handleError(SocketSupport::getLastSocketError(), log);
}

bool Socket::handleSendRecvError(Log* log, bool& returnZero)
{
    SocketSupport::ErrorCode error = SocketSupport::getLastSocketError();

    if (error == SocketSupport::ErrorConnectionReset) {
        SocketSupport::logSocketError(log, error, Log::LevelTrace);
        returnZero = true;
        return false;
    }

    returnZero = false;
    return handleError(error, log);
}

bool Socket::handleError(SocketSupport::ErrorCode errorCode, Log* log)
{
    _lastError = errorCode;

    if (!isCreated()) {
        // The socket was probably closed by another thread.
        return false;
    }

    if (_lastError == SocketSupport::ErrorInterrupt) {
        if (_shouldRetry) {
            return true;
        }
    }

    if (_lastError == SocketSupport::ErrorWouldBlock) {
        log->trace("Socket would block.");
        return false;
    }

    if (log) {
        SocketSupport::logSocketError(log, _lastError);
    }

    return false;
}

void Socket::setCloseSignal(SignalSocket* closeSignal)
{
    _closeSignal = closeSignal;
}

SignalSocket* Socket::getCloseSignal() const
{
    return static_cast<SignalSocket*>(_closeSignal.get());
}

Socket* Socket::getCloseSignalSocket() const
{
    if (SignalSocket* signalSocket = getCloseSignal()) {
        return &signalSocket->getSocket();
    }

    return NULL;
}

bool Socket::close(Log* log)
{
    bool success = true;

    // Clear _handle before closing the socket so that a thread waiting on accept can see the change.
    Handle handleWas = _handle;
    _handle = SocketSupport::invalidHandle;

    if (_shouldClose) {
        for (;;) {
            if (SocketSupport::closeSocket(handleWas) == 0) {
                break;
            }

            if (!handleError(log)) {
                success = false;
                break;
            }
        }

        _shouldClose = false;
    }

    return success;
}

bool Socket::create(int family, int type, int protocol, Log* log, const Options& options)
{
    PRIME_ASSERT(!isCreated());

    const bool forceNoInherit = !options.getChildProcessInherit();
    Handle sh;
    for (;;) {
        sh = SocketSupport::createSocket(family, type, protocol, forceNoInherit);
        if (sh != SocketSupport::invalidHandle) {
            setHandle(sh, true);
            return true;
        }

        if (!handleError(log)) {
            return false;
        }
    }
}

bool Socket::createTCPIP4(Log* log, const Options& options)
{
    return create(AF_INET, SOCK_STREAM, IPPROTO_TCP, log, options);
}

bool Socket::createUDPIP4(Log* log, const Options& options)
{
    return create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, log, options);
}

bool Socket::createForAddress(const SocketAddress& address, int socketType, int protocol, Log* log, const Options& options)
{
    return create(address.getFamily(), socketType, protocol, log, options);
}

bool Socket::connect(const SocketAddress& address, Log* log)
{
    PRIME_ASSERT(isCreated());
    PRIME_ASSERT(!address.isNull());

    for (;;) {
        if (::connect(getHandle(), address.get(), address.getLength()) >= 0) {
            return true;
        }

        if (!handleError(log)) {
            return false;
        }
    }
}

bool Socket::connect(const SocketAddress& address, int milliseconds, Log* log)
{
    PRIME_ASSERT(isCreated());
    PRIME_ASSERT(!address.isNull());

    if (milliseconds < 0) {
        return connect(address, log);
    }

    if (!setNonBlocking(true, log)) {
        return false;
    }

    bool result = nonBlockingConnect(address, milliseconds, log);

    if (!setNonBlocking(false, log)) {
        return false;
    }

    return result;
}

bool Socket::nonBlockingConnect(const SocketAddress& address, int milliseconds, Log* log)
{
    for (;;) {
        if (::connect(getHandle(), address.get(), address.getLength()) >= 0) {
            return true;
        }

        if (SocketSupport::getLastSocketError() != EINPROGRESS) {
            if (!handleError(log)) {
                return false;
            }

            continue;
        }

        for (;;) {
            struct timeval timeout;
            timeout.tv_sec = milliseconds / 1000;
            timeout.tv_usec = (milliseconds % 1000) * 1000;

            fd_set readSet;
            Handle maxHandle = _handle;
            FD_ZERO(&readSet);
            FD_SET(_handle, &readSet);
            if (_closeSignal) {
                Handle closeHandle = getCloseSignal()->getSocket().getHandle();
                FD_SET(closeHandle, &readSet);

                if (closeHandle > maxHandle) {
                    maxHandle = closeHandle;
                }
            }

            // TODO: also wait on the signal socket?

            int selected = ::select(maxHandle + 1, NULL, &readSet, NULL, milliseconds < 0 ? NULL : &timeout);
            if (selected < 0) {
                if (!handleError(log)) {
                    return false;
                }

                continue;
            }

            if (selected == 0) {
                // Timeout
                handleError(ETIMEDOUT, log);
                return false; // TODO: need an enum
            }

            if (!FD_ISSET(_handle, &readSet)) {
                // Close signal
                handleError(EINPROGRESS, log);
                return false;
            }

            int error = 0;
            socklen_t errorLen = static_cast<socklen_t>(sizeof(error));
            if (getsockopt(_handle, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &errorLen) < 0) {
                handleError(log);
                return false;
            }

            if (errorLen != static_cast<socklen_t>(sizeof(error))) {
                log->error("getsockopt error length incorrect.");
                return false;
            }

            if (!error) {
                return true;
            }

            handleError(error, log);
            return false;
        }
    }
}

Socket::WaitResult Socket::select(int milliseconds, SelectSocket* reads, SelectSocket* writes,
    SelectSocket* errors, Log* log)
{
    for (;;) {
        fd_set readSet, writeSet, errorSet;
        struct Set {
            SelectSocket* selectSockets;
            fd_set* fdSet;
        } sets[3] = {
            { reads, &readSet },
            { writes, &writeSet },
            { errors, &errorSet }
        };

#ifdef _MSC_VER
// "conditional expression is constant"
#pragma warning(disable : 4127)
#endif

        Handle maxSH = 0;

        for (int setNumber = 0; setNumber != 3; ++setNumber) {
            Set& set = sets[setNumber];
            FD_ZERO(set.fdSet);

            for (SelectSocket* ptr = set.selectSockets; ptr && ptr->socket; ++ptr) {
                Handle sh = ptr->socket->getHandle();
                ptr->isSet = false;
                maxSH = Max(sh, maxSH);
                FD_SET(sh, set.fdSet);
            }
        }

#ifdef _MSC_VER
#pragma warning(default : 4127)
#endif

        struct timeval timeout;
        timeout.tv_sec = milliseconds / 1000;
        timeout.tv_usec = (milliseconds % 1000) * 1000;

        int result = SocketSupport::selectSocket((int)maxSH + 1, &readSet, &writeSet, &errorSet, milliseconds < 0 ? NULL : &timeout);

        if (result > 0) {
            for (int setNumber = 0; setNumber != 3; ++setNumber) {
                Set& set = sets[setNumber];

                for (SelectSocket* ptr = set.selectSockets; ptr && ptr->socket; ++ptr) {
                    Handle sh = ptr->socket->getHandle();
                    ptr->isSet = FD_ISSET(sh, set.fdSet) ? true : false;
                }
            }

            return WaitResultOK;
        }

        if (result < 0) {
            if (handleError(log)) {
                continue;
            }

            return WaitResultCancelled;
        }

        return WaitResultTimedOut;
    }
}

Socket::WaitResult Socket::waitRecv(int milliseconds, Log* log)
{
    PRIME_ASSERT(isCreated());
    SelectSocket reads[] = {
        { this, false },
        { _closeSignal ? &getCloseSignal()->getSocket() : NULL, false },
        { NULL, false }
    };

    WaitResult result = select(milliseconds, reads, NULL, NULL, log);

    if (result != WaitResultOK) {
        return result;
    }

    if (reads[1].isSet) {
        return WaitResultCancelled;
    }

    return WaitResultOK;
}

Socket::WaitResult Socket::waitSend(int milliseconds, Log* log)
{
    PRIME_ASSERT(isCreated());
    SelectSocket writes[] = { { this, false }, { NULL, false } };
    SelectSocket reads[] = {
        { _closeSignal ? &getCloseSignal()->getSocket() : NULL, false },
        { NULL, false }
    };

    WaitResult result = select(milliseconds, reads, writes, NULL, log);

    if (result != WaitResultOK) {
        return result;
    }

    if (reads[0].isSet) {
        return WaitResultCancelled;
    }

    return WaitResultOK;
}

ptrdiff_t Socket::recv(void* buffer, size_t request, Log* log)
{
    PRIME_ASSERT(isCreated());

    bool returnZero = false;
    for (;;) {
        int recvd = (int)::recv(getHandle(), (char*)buffer, (SocketSupport::BufferSize)request, 0);

        if (recvd < 0 && handleSendRecvError(log, returnZero)) {
            continue;
        }

        return returnZero ? 0 : recvd;
    }
}

ptrdiff_t Socket::send(const void* buffer, size_t request, Log* log)
{
    PRIME_ASSERT(isCreated());

    bool returnZero = false;
    for (;;) {
        int wrote = (int)::send(getHandle(), (const char*)buffer, (SocketSupport::BufferSize)request, 0);

        if (wrote < 0 && handleSendRecvError(log, returnZero)) {
            continue;
        }

        return returnZero ? 0 : wrote;
    }
}

bool Socket::sendAll(const void* data, size_t length, Log* log)
{
    if (!length) {
        return send(data, length, log) == 0;
    }

    while (length) {
        ptrdiff_t sent = send(data, length, log);
        if (sent < 0) {
            return false;
        }

        if (sent == 0) {
            return false; // socket closed.
        }

        data = (const char*)data + sent;
        length -= sent;
    }

    return true;
}

bool Socket::bind(const SocketAddress& address, Log* log)
{
    PRIME_ASSERT(isCreated());

    for (;;) {
        if (::bind(getHandle(), address.get(), address.getLength()) == 0) {
            return true;
        }

        if (!handleError(log)) {
            return false;
        }
    }
}

bool Socket::listen(Log* log, int queue)
{
    PRIME_ASSERT(isCreated());

    if (queue < 0) {
        queue = SocketSupport::maxListenBacklog;
    }

    for (;;) {
        if (::listen(getHandle(), queue) == 0) {
            return true;
        }

        if (!handleError(log)) {
            return false;
        }
    }
}

bool Socket::accept(Socket& socket, SocketAddress& addr, Log* log, const Options& options)
{
    PRIME_ASSERT(isCreated());

    const bool forceNoInherit = !options.getChildProcessInherit();

    for (;;) {
        char addrBuffer[SocketSupport::maxAddressLength];
        memset(addrBuffer, 0, sizeof(addrBuffer));

        SocketSupport::AddressLength bufferLength = (SocketSupport::AddressLength)sizeof(addrBuffer);

        Handle client = SocketSupport::acceptSocket(getHandle(), (sockaddr*)addrBuffer, &bufferLength, forceNoInherit);
        if (client != SocketSupport::invalidHandle) {
            socket.setHandle(client, true);
            addr.set(addrBuffer, bufferLength);
            return true;
        }

        if (!handleError(log)) {
            return false;
        }
    }
}

ptrdiff_t Socket::recvFrom(SocketAddress& addr, void* buffer, size_t bufferSize, Log* log)
{
    PRIME_ASSERT(isCreated());

    for (;;) {
        char addrBuffer[SocketSupport::maxAddressLength];
        memset(addrBuffer, 0, sizeof(addrBuffer));

        SocketSupport::AddressLength bufferLength = (SocketSupport::AddressLength)sizeof(addrBuffer);

        int recvd = (int)::recvfrom(getHandle(), (char*)buffer, (SocketSupport::BufferSize)bufferSize, 0, (sockaddr*)addrBuffer, &bufferLength);
        if (recvd >= 0) {
            addr.set(addrBuffer, bufferLength);
            return recvd;
        }

        if (!handleError(log)) {
            return -1;
        }
    }
}

ptrdiff_t Socket::sendTo(const SocketAddress& toAddress, const void* buffer, size_t byteCount, Log* log)
{
    PRIME_ASSERT(isCreated());

    for (;;) {
        int sent = (int)::sendto(getHandle(), (const char*)buffer, (SocketSupport::BufferSize)byteCount, 0, toAddress.get(), toAddress.getLength());

        if (sent < 0 && handleError(log)) {
            continue;
        }

        return sent;
    }
}

bool Socket::getRemoteAddress(SocketAddress& addr, Log* log)
{
    PRIME_ASSERT(isCreated());

    for (;;) {
        char addrBuffer[SocketSupport::maxAddressLength];
        memset(addrBuffer, 0, sizeof(addrBuffer));

        SocketSupport::AddressLength bufferLength = (SocketSupport::AddressLength)sizeof(addrBuffer);

        if (getpeername(getHandle(), (sockaddr*)addrBuffer, &bufferLength) >= 0) {
            addr.set(addrBuffer, bufferLength);
            return true;
        }

        if (!handleError(log)) {
            return false;
        }
    }
}

bool Socket::getLocalAddress(SocketAddress& addr, Log* log)
{
    PRIME_ASSERT(isCreated());

    for (;;) {
        char addrBuffer[SocketSupport::maxAddressLength];
        memset(addrBuffer, 0, sizeof(addrBuffer));

        SocketSupport::AddressLength bufferLength = (SocketSupport::AddressLength)sizeof(addrBuffer);

        if (getsockname(getHandle(), (sockaddr*)addrBuffer, &bufferLength) >= 0) {
            addr.set(addrBuffer, bufferLength);
            return true;
        }

        if (!handleError(log)) {
            return false;
        }
    }
}

bool Socket::setNonBlocking(bool value, Log* log)
{
    PRIME_ASSERT(isCreated());

    for (;;) {
        if (SocketSupport::setSocketNonBlocking(getHandle(), value)) {
            return true;
        }

        if (!handleError(log)) {
            return false;
        }
    }
}

bool Socket::setBroadcast(bool value, Log* log)
{
    PRIME_ASSERT(isCreated());

    int parm = value ? 1 : 0;

    for (;;) {
        int res = setsockopt(getHandle(), SOL_SOCKET, SO_BROADCAST, (char*)&parm, sizeof(parm));
        if (res >= 0) {
            return true;
        }

        if (!handleError(log)) {
            return false;
        }
    }
}

bool Socket::setReuseAddress(bool value, Log* log)
{
    PRIME_ASSERT(isCreated());

    int parm = value ? 1 : 0;

    for (;;) {
        int res = setsockopt(getHandle(), SOL_SOCKET, SO_REUSEADDR, (char*)&parm, sizeof(parm));
        if (res >= 0) {
            return true;
        }

        if (!handleError(log)) {
            return false;
        }
    }
}

void Socket::takeOwnership(Socket& from)
{
    if (this != &from) {
        close(Log::getNullLog());

        _handle = from._handle;
        _shouldClose = from._shouldClose;
        _shouldRetry = from._shouldRetry;
        _lastError = from._lastError;
        _closeSignal = from._closeSignal;

        from._shouldClose = false;
    }
}
}
