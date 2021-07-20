// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXCLOSEONEXEC_H
#define PRIME_UNIX_UNIXCLOSEONEXEC_H

#include "../Config.h"

namespace Prime {

/// Deal with UNIX handles being inherited by child processes by default. When you open a kernel handle (e.g.,
/// a file descriptor or socket), place the call to open() (or socket(), etc.) within a scope where a
/// UnixCloseOnExec::ScopedLock exists, and call closeOnExec() on the handle before the ScopedLock is
/// destructed/unlocked. Synchronisation is needed to ensure a process isn't launched between a handle being
/// opened and the handle being marked as close-on-exec.
class PRIME_PUBLIC UnixCloseOnExec {
public:
    static void closeOnExec(int fd);

    /// Not necessary for C++11.
    static void globalInit();

    static void globalShutdown();

    /// Example:
    ///     UnixCloseOnExec::ScopedLock execLock;
    ///
    ///     int fd;
    ///     for (;;) {
    ///         fd = ::open(path, unixFlags, permissions);
    ///         if (fd >= 0) {
    ///             break;
    ///         }
    ///
    ///         if (errno != EINTR) {
    ///             log->logErrno(errno);
    ///             return false;
    ///         }
    ///     }
    ///
    ///     UnixCloseOnExec::closeOnExec(fd);
    class ScopedLock {
    public:
        explicit ScopedLock(bool lock)
        {
            _locked = lock;
            if (lock) {
                UnixCloseOnExec::lock();
            }
        }

        ScopedLock()
        {
            UnixCloseOnExec::lock();
            _locked = true;
        }

        ~ScopedLock() { unlock(); }

        void lock()
        {
            if (!_locked) {
                UnixCloseOnExec::lock();
                _locked = true;
            }
        }

        void unlock()
        {
            if (_locked) {
                UnixCloseOnExec::unlock();
                _locked = false;
            }
        }

    private:
        bool _locked;
    };

    // Use a ScopedLock if possible, to take advantage of RAII.
    static void lock();
    static void unlock();

protected:
    friend class ScopedLock;
};
}

#endif
