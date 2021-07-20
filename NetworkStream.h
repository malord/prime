// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_NETWORKSTREAM_H
#define PRIME_NETWORKSTREAM_H

#include "Stream.h"

namespace Prime {

/// Extend Stream with methods specific to network streams.
class PRIME_PUBLIC NetworkStream : public Stream {
    PRIME_DECLARE_UID_CAST(Stream, 0x7433cb1b, 0xff49484d, 0x9b30df29, 0xe15831e8)

public:
    NetworkStream() { }

    /// Set the timeout to apply to readSome().
    virtual void setReadTimeout(int milliseconds) = 0;

    /// Milliseconds.
    virtual int getReadTimeout() const = 0;

    /// Set the timeout to apply to writeSome().
    virtual void setWriteTimeout(int milliseconds) = 0;

    /// Milliseconds.
    virtual int getWriteTimeout() const = 0;

    /// Result of a wait operation.
    enum WaitResult {
        /// The wait operation was aborted, possibly because the socket was closed.
        WaitResultCancelled,

        /// Data became available.
        WaitResultOK,

        /// Timeout reached.
        WaitResultTimedOut
    };

    /// Wait, up to the specified number of milliseconds, for data to become available to read. Returns a member
    /// of the WaitResult enumeration. Specify -1 for milliseconds to wait forever. Note that the implementation's
    /// readSome() is expected to call waitReadTimeout(), so you should only call this method directly if you
    /// need to check for a timeout and don't want readSome() logging an error.
    virtual WaitResult waitRead(int milliseconds, Log* log) = 0;

    /// Wait, up to the specified number of milliseconds, for space to become available in the send buffer.
    /// Returns a member of the WaitResult enumeration. Specify -1 for milliseconds to wait forever. Note that
    /// the implementation's writeSome() is expected to call waitWriteTimeout(), so you should only call this
    /// method directly if you need to check for a timeout and don't want writeSome() logging an error.
    virtual WaitResult waitWrite(int milliseconds, Log* log) = 0;

    /// An implementation should call this in its readSome() method. It calls waitRead(getReadTimeout(), log)
    /// and emits an error to log if timeout occurs. Returns false on timeout or socket close, true if data is
    /// available to read.
    bool waitReadTimeout(Log* log);

    /// An implementation should call this in its writeSome() method. It calls waitWrite(getWriteTimeout(), log)
    /// and emits an error to log if timeout occurs. Returns false on timeout or socket close, true if buffer
    /// space is available to write to.
    bool waitWriteTimeout(Log* log);

private:
    PRIME_UNCOPYABLE(NetworkStream);
};
}

#endif
