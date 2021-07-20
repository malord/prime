// Copyright 2000-2021 Mark H. P. Lord

#include "SocketStream.h"
#include "FileStream.h"
#include "Substream.h"
#include <string.h>
#ifdef PRIME_OS_WINDOWS
#include <Mswsock.h>
#endif

namespace Prime {

PRIME_DEFINE_UID_CAST(SocketStream)

bool SocketStream::close(Log* log)
{
    // It's not an error to call this with an already closed socket.
    return _socket.close(log);
}

ptrdiff_t SocketStream::readSome(void* buffer, size_t maximumBytes, Log* log)
{
    PRIME_ASSERT(isCreated());

    if (!waitReadTimeout(log)) {
        return -1;
    }

    return _socket.recv(buffer, maximumBytes, log);
}

ptrdiff_t SocketStream::writeSome(const void* memory, size_t maximumBytes, Log* log)
{
    PRIME_ASSERT(isCreated());

    if (!waitWriteTimeout(log)) {
        return -1;
    }

    return _socket.send(memory, maximumBytes, log);
}

bool SocketStream::copyFrom(Stream* source, Log* sourceLog, Offset length, Log* destLog, size_t bufferSize,
    void* buffer)
{
    (void)source;
    (void)sourceLog;
    (void)destLog;
    (void)bufferSize;
    (void)buffer;

    Stream::Offset offset = source->getOffset(Log::getNullLog());
    if (offset < 0) {
        return NetworkStream::copyFrom(source, sourceLog, length, destLog, bufferSize, buffer);
    }

    if (length < 0) {
        Stream::Offset size = source->getSize(Log::getNullLog());
        if (size < 0) {
            return NetworkStream::copyFrom(source, sourceLog, length, destLog, bufferSize, buffer);
        }

        length = size - offset;
    }

    // Get down to the actual file handle. This lets us send deflate()d content directly from a zip file.
    for (;;) {
        Substream* substream = UIDCast<Substream>(source);
        if (!substream) {
            break;
        }

        source = substream->getUnderlyingStream();
        offset += substream->getBaseOffset();
    }

#if defined(PRIME_OS_LINUX)

    UnixFileStream* unixStream = UIDCast<UnixFileStream>(source);

    if (unixStream && (Offset)(size_t)length == length) {
        size_t len = (size_t)length;
        off_t ofs = Narrow<off_t>(offset);
        // sourceLog->trace("Using sendfile...");
        ssize_t result = sendfile(_socket.getHandle(), unixStream->getHandle(), &ofs, len);
        if (result != len) {
            destLog->logErrno(errno);
            return false;
        }

        return true;
    }

#elif defined(PRIME_OS_BSD)

    if (UnixFileStream* unixStream = UIDCast<UnixFileStream>(source)) {
        off_t len = length;
        // sourceLog->trace("Using sendfile...");
        int result = sendfile(unixStream->getHandle(), _socket.getHandle(), offset, &len, NULL, 0);
        if (result != 0 || len != length) {
            destLog->logErrno(errno);
            return false;
        }

        return true;
    }

#elif defined(PRIME_OS_WINDOWS) && PRIME_WINVER >= PRIME_WINDOWS_VISTA

    if (WindowsFileStream* windowsStream = UIDCast<WindowsFileStream>(source)) {
        if (!TransmitFile(_socket.getHandle(), windowsStream->getHandle(), Narrow<DWORD>(length), 0, NULL, NULL, 0)) {
            destLog->logWindowsError(WSAGetLastError());
            return false;
        }
    }

#endif

    return NetworkStream::copyFrom(source, sourceLog, length, destLog, bufferSize, buffer);
}

NetworkStream::WaitResult SocketStream::waitRead(int milliseconds, Log* log)
{
    return mapWaitResult(_socket.waitRecv(milliseconds, log));
}

NetworkStream::WaitResult SocketStream::waitWrite(int milliseconds, Log* log)
{
    return mapWaitResult(_socket.waitSend(milliseconds, log));
}

}
