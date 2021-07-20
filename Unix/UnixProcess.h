// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXPROCESS_H
#define PRIME_UNIX_UNIXPROCESS_H

#include "../Log.h"
#include "../ProcessBase.h"
#include <unistd.h>

namespace Prime {

class PRIME_PUBLIC UnixProcess : public ProcessBase {
public:
    /// e.g., Process::Stream processStream(&process);
    typedef ProcessBase::ProcessStream<UnixProcess> Stream;

    UnixProcess();

    /// If the process hasn't been detached, calls wait(), then calls detach().
    ~UnixProcess();

    /// Launch a process with the supplied arguments, where the command to be run is in the first argument.
    /// Returns false if the process couldn't be  launched. Note that on some platforms, if processWait isn't used
    /// then this function may return successfully but the process won't have run. You need to check for an exit
    /// code of ExitCodeFailed.
    bool create(const Argument* args, size_t nargs, const Options& options, Log* log);

    /// Detach the process. You must call this for every successful call to create(), even if you call wait(),
    /// join() or kill(). It is safe to call this without calling create().
    void detach();

    /// Wait for the process to finish. Returns the exit code of the process. It is safe to call this if no
    /// process has been created.
    int wait(Log* log);

    /// Returns the exit code of a process which has finished. Call this after calling wait(), creating a process
    /// with processWait or after finding that isRunning() returns false. If the process failed to launch,
    /// ExitCodeFailed is returned. If the process was killed, ExitCodeKilled is returned.
    int getExitCode();

    /// Returns true if a process is still running.
    bool isRunning();

    /// Read bytes from a process's stdout (if enabled in the options). Returns < 0 on error, 0 if the process
    /// has finished, otherwise returns the number of bytes read.
    ptrdiff_t read(void* buffer, size_t bufferSize, Log* log);

    /// Write bytes to a process's stdin (if enabled in the options). Returns < 0 on error, 0 if the process
    /// has finished, otherwise returns the number of bytes written
    ptrdiff_t write(const void* bytes, size_t byteCount, Log* log);

    /// Write all the specified bytes to the process and return false on error.
    bool writeExact(const void* bytes, size_t byteCount, Log* log);

    /// Flush the write buffer of a process created for read/write.
    bool flush(Log* log);

    /// Close the stdin of a process created with the write option.
    bool endWrite(Log* log);

    /// Returns the stdout file descriptor for a running process (i.e., the file descriptor your read() to read
    /// the process's stdout).
    int getStdout() const { return _theirStdout; }

    /// Returns the pid of a running process.
    pid_t getPid() const { return _pid; }

    /// Kill a process using a UNIX signal. You can then wait for it to quit, or detach it.
    bool kill(int sig, Log* log);

private:
    bool createDirect(const Argument* args, size_t nargs, const Options& options, Log* log);

    bool createViaShell(const Argument* args, size_t nargs, const Options& options, Log* log);
    bool createViaShell(const char* cmdline, const Options& options, Log* log);
    bool createViaShellAppend(const char* cmdline, const char* append, const Options& options, Log* log);

    bool create(const char* executable, const char* const* argv, const Options& options, Log* log);

    void closePipes();

    pid_t _pid;
    int _exitCode;
    int _theirStdin;
    int _theirStdout;

    PRIME_UNCOPYABLE(UnixProcess);
};

}

#endif
