// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSPROCESS_H
#define PRIME_WINDOWS_WINDOWSPROCESS_H

#include "../Log.h"
#include "../ProcessBase.h"
#include "WindowsConfig.h"

namespace Prime {

class PRIME_PUBLIC WindowsProcess : public ProcessBase {
public:
    /// e.g., Process::Stream processStream(&process);
    typedef ProcessBase::ProcessStream<WindowsProcess> Stream;

    WindowsProcess();

    /// If the process hasn't been detached, calls wait(), then calls detach().
    ~WindowsProcess();

    /// Launch a process with the supplied arguments, where the command to be run is in the first argument.
    /// Returns false if the process couldn't be  launched. Note that on some platforms, if processWait isn't used
    /// then this function may return successfully but the process won't have run. You need to check for an exit
    /// code of ExitCodeFailed.
    bool create(const Argument* args, size_t nargs, const Options& options, Log* log);

    /// Detach the process. You must call this for every successful call to create(), even if you call wait(),
    /// join() or kill(). It is safe to call this without calling create().
    void detach();

    /// Wait for the process to finish. Returns the exit code of the process.
    int wait(Log* log);

    /// Returns the exit code of a process which has finished. Call this after calling wait(), creating a process
    /// with processWait or after finding that isRunning() returns false. If the process failed to launch,
    /// ExitCodeFailed is returned. If the process was killed, ExitCodeKilled is returned.
    int getExitCode() { return _exitCode; }

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

private:
    bool createWithCommandLine(const char* cmdline, const Options& options, Log* log);

    void closeEverything(Log* log);

    bool closePipes(Log* log);

    bool waitTimeout(DWORD timeout, Log* log);

    struct Pipes {
        HANDLE outRead;
        HANDLE outWrite;
        HANDLE inRead;
        HANDLE inWrite;
    };

    int _exitCode;
    PROCESS_INFORMATION _processInfo;
    Pipes _pipes;

    PRIME_UNCOPYABLE(WindowsProcess);
};
}

#endif
