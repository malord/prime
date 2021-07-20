// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PROCESSBASE_H
#define PRIME_PROCESSBASE_H

#include "ScopedPtr.h"
#include "Stream.h"

namespace Prime {

/// Base class for the platform specific Process implementations.
class PRIME_PUBLIC ProcessBase : public RefCounted {
public:
    ProcessBase() { }

    struct PRIME_PUBLIC Argument {
    private:
        std::string _argument;

        bool _wildcard;
        bool _verbatim;

    public:
        Argument();

        Argument(std::string argument);

        Argument(const char* argument);

        ~Argument();

        Argument(const Argument& copy);

        Argument& operator=(const Argument& copy);

        Argument& operator=(std::string argument);

        Argument& operator=(const char* argument);

        const std::string& getArgument() const { return _argument; }

        Argument& set(std::string argument);

        /// A wildcard command line argument. * and ? are not escaped, and the argument will not be enclosed in
        /// quotes.
        Argument& setWildcard(bool value = true)
        {
            _wildcard = value;
            return *this;
        }
        bool isWildcard() const { return _wildcard; }

        /// The argument is passed through to the process unchanged. Note that if the argument contains spaces,
        /// multiple arguments may (platform dependant) be received by the process being invoked.
        Argument& setVerbatim(bool value = true)
        {
            _verbatim = value;
            return *this;
        }
        bool isVerbatim() const { return _verbatim; }
    };

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _wait(false)
            , _read(false)
            , _write(false)
            , _shell(false)
            , _redirectStderrToStdout(false)
            , _redirectStderrToNull(false)
            , _redirectStdoutToNull(false)
            , _showConsole(false)
            , _logCommandLine(GetDeveloperMode())
        {
        }

        Options(const Options& copy) { operator=(copy); }

        /// Wait for the process to finish before returning. You can then get the exit code with getExitCode().
        /// On some platforms, this may be required to be true.
        Options& setWait(bool value = true)
        {
            _wait = value;
            return *this;
        }
        bool getWait() const { return _wait; }

        /// Read the process's stdout with the read() method. Shouldn't be used with setWrite since that can
        /// cause deadlock.
        Options& setRead(bool value = true)
        {
            _read = value;
            return *this;
        }
        bool getRead() const { return _read; }

        /// Write to the process's stdin with the write() method. Shouldn't be used with setRead since that can
        /// cause deadlock.
        Options& setWrite(bool value = true)
        {
            _write = value;
            return *this;
        }
        bool getWrite() const { return _write; }

        /// Invoke the executable via the shell, rather than directly.
        Options& setUseShell(bool value = true)
        {
            _shell = value;
            return *this;
        }
        bool getUseShell() const { return _shell; }

        /// Redirect the process's stderr to stdout.
        Options& setRedirectStderrToStdout(bool value = true)
        {
            _redirectStderrToStdout = value;
            return *this;
        }
        bool getRedirectStderrToStdout() const { return _redirectStderrToStdout; }

        /// Redirect the process's stderr to /dev/null.
        Options& setRedirectStderrToNull(bool value = true)
        {
            _redirectStderrToNull = value;
            return *this;
        }
        bool getRedirectStderrToNull() const { return _redirectStderrToNull; }

        /// Redirect the process's stdout to /dev/null.
        Options& setRedirectStdoutToNull(bool value = true)
        {
            _redirectStdoutToNull = value;
            return *this;
        }
        bool getRedirectStdoutToNull() const { return _redirectStdoutToNull; }

        /// Should console application's console be visible.
        Options& setShowConsole(bool value = true)
        {
            _showConsole = value;
            return *this;
        }
        bool getShowConsole() const { return _showConsole; }

        /// Should we log the command line?
        Options& setLogCommandLine(bool value = true)
        {
            _logCommandLine = value;
            return *this;
        }
        bool getLogCommandLine() const { return _logCommandLine; }

    private:
        bool _wait;
        bool _read;
        bool _write;
        bool _shell;
        bool _redirectStderrToStdout;
        bool _redirectStderrToNull;
        bool _redirectStdoutToNull;
        bool _showConsole;
        bool _logCommandLine;
    };

    enum ExitCodes {
        /// Returned as the exit code if the process was killed.
        ExitCodeKilled = -10001,

        /// Returned as the exit code if the process launch failed.
        ExitCodeFailed = -10000,
    };

    static bool isSpecialExitCode(int exitCode)
    {
        return exitCode == ExitCodeFailed || exitCode == ExitCodeKilled;
    }

    template <typename Process>
    class ProcessStream : public Stream {
    public:
        explicit ProcessStream(Process* process)
            : _process(process)
        {
        }

        virtual ptrdiff_t readSome(void* buffer, size_t maxBytes, Log* log) PRIME_OVERRIDE
        {
            return _process->read(buffer, maxBytes, log);
        }

        virtual ptrdiff_t writeSome(const void* bytes, size_t maxBytes, Log* log) PRIME_OVERRIDE
        {
            return _process->write(bytes, maxBytes, log);
        }

        bool end(Log* log)
        {
            return _process->endWrite(log);
        }

        virtual bool close(Log* log) PRIME_OVERRIDE
        {
            bool success = _process->endWrite(log);

            _process = NULL;

            return success;
        }

    private:
        RefPtr<Process> _process;
    };

    PRIME_UNCOPYABLE(ProcessBase);
};

}

#endif
