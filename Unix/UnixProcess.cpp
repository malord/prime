// Copyright 2000-2021 Mark H. P. Lord

#include "UnixProcess.h"
#include "../StringUtils.h"
#include "UnixCloseOnExec.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#if defined(PRIME_OS_HAS_POSIX_SPAWN)
#include <spawn.h>
#elif defined(PRIME_OS_HAS_VFORK)
#define FORK_API vfork
#define FORK_API_NAME "vfork"
#define FORK_EXIT_API _exit
#else
#define FORK_API fork
#define FORK_API_NAME "fork"
#define FORK_EXIT_API _exit
#endif

// If defined, all file handles except stdin, stdout and stderr wil be closed in the child process as soon as it has
// forked. This is important as it prevents the child process holding handles intended for another process, or for
// the parent process, which could cause the other process to deadlock waiting for a file/pipe to be closed. This is
// disabled by default because closing all file descriptors >= 3 is expensive and because
// fcntl(fd, F_SETFD, FD_CLOEXEC), which makes a descriptor close-on-exec, is a better solution if available
// (see UnixCloseOnExec).
//#define CLOSE_ALL_CHILD_HANDLES

#if !defined(F_SETFD) && !defined(CLOSE_ALL_CHILD_HANDLES)
// Must use CLOSE_ALL_CHILD_HANDLES if F_SETFD is not available.
#define CLOSE_ALL_CHILD_HANDLES
#endif

namespace {

using namespace Prime;

const int stdinHandle = 0;
const int stdoutHandle = 1;
const int stderrHandle = 2;
const int readPipe = 0;
const int writePipe = 1;

inline bool NeedCommandLineHack(const UnixProcess::Options& options)
{
    return options.getRedirectStderrToStdout() || options.getRedirectStderrToNull() || options.getRedirectStdoutToNull();
}

bool NeedsEscapingForShell(char c, bool inDoubleQuotes, bool wildcard)
{
    static const char safe[] = "@-_+=,./";
    static const char safeWhenQuoted[] = "@-_+=,.'#;:{}[]()/|<> \t";

    return !(ASCIIIsAlpha(c) || ASCIIIsDigit(c) || (wildcard && (c == '?' || c == '*')) || strchr(inDoubleQuotes ? safeWhenQuoted : safe, c));
}

void BuildCommandLine(std::string& cmdline, const UnixProcess::Argument* args, size_t nargs)
{
    for (size_t i = 0; i != nargs; ++i) {
        const UnixProcess::Argument* arg = &args[i];

        if (i != 0) {
            cmdline += ' ';
        }

        if (arg->isVerbatim()) {
            cmdline.append(arg->getArgument());
            continue;
        }

        if (arg->getArgument().empty()) {
            // Empty argument.
            cmdline.append("\"\"", 2);
            continue;
        }

        bool wildcard = arg->isWildcard();

        bool containsSpace = false;
        bool containsDoubleQuote = false;
        bool containsSingleQuote = false;
        bool needsEscaping = false;
        for (const char* ptr = arg->getArgument().c_str(); *ptr; ++ptr) {
            if (*ptr == ' ' || *ptr == '\t') {
                containsSpace = true;
            } else if (*ptr == '"') {
                containsDoubleQuote = true;
            } else if (*ptr == '\'') {
                containsSingleQuote = true;
            } else if (NeedsEscapingForShell(*ptr, false, wildcard)) {
                needsEscaping = true;
            }
        }

        if (!needsEscaping && !containsSpace && !containsSingleQuote && !containsDoubleQuote) {
            cmdline.append(arg->getArgument());
            continue;
        }

        if (!containsSingleQuote && !wildcard) {
            // Enclose the whole argument in single quotes.
            cmdline.push_back('\'');
            cmdline.append(arg->getArgument());
            cmdline.push_back('\'');
            continue;
        }

        bool useDoubleQuotes = containsSpace && !wildcard;

        if (useDoubleQuotes) {
            cmdline += '"';
        }

        const char* last = arg->getArgument().c_str();
        for (const char* ptr = arg->getArgument().c_str();; ++ptr) {
            if (NeedsEscapingForShell(*ptr, useDoubleQuotes, wildcard) || !*ptr) {
                cmdline.append(last, (size_t)(ptr - last));

                if (!*ptr) {
                    break;
                }

                cmdline.push_back('\\');
                cmdline.push_back(*ptr);

                last = ptr + 1;
            }
        }

        if (useDoubleQuotes) {
            cmdline += '"';
        }
    }
}
}

namespace Prime {

UnixProcess::UnixProcess()
{
    _pid = -1;
    _exitCode = ExitCodeFailed;
    _theirStdout = _theirStdin = -1;
}

UnixProcess::~UnixProcess()
{
    if (_pid >= 0) {
        wait(Log::getNullLog());
        detach();
    }

    closePipes();
}

bool UnixProcess::create(const Argument* args, size_t nargs, const Options& options, Log* log)
{
    PRIME_ASSERT(_pid < 0); // Already attached to a process.

    if (options.getUseShell()) {
        return createViaShell(args, nargs, options, log);
    }

    // This is somewhat rubbish - we could do wildcard expansion ourselves.
    for (size_t i = 0; i != nargs; ++i) {
        if (args[i].isWildcard()) {
            return create(args, nargs, Options(options).setUseShell(), log);
        }
    }

    return createDirect(args, nargs, options, log);
}

bool UnixProcess::createDirect(const Argument* args, size_t nargs, const Options& options, Log* log)
{
    std::vector<const char*> argv(nargs + 1);

    for (size_t i = 0; i != nargs; ++i) {
        argv[i] = args[i].getArgument().c_str();
    }

    argv[nargs] = NULL;

    return create(argv[0], argv.data(), options, log);
}

bool UnixProcess::createViaShell(const Argument* args, size_t nargs, const Options& options, Log* log)
{
    std::string cmdline;
    BuildCommandLine(cmdline, args, nargs);

    return createViaShell(cmdline.c_str(), options, log);
}

bool UnixProcess::createViaShell(const char* cmdline, const Options& options, Log* log)
{
    if (options.getRedirectStderrToStdout()) {
        return createViaShellAppend(cmdline, " 2>&1", Options(options).setRedirectStderrToStdout(false), log);
    }

    if (options.getRedirectStderrToNull()) {
        return createViaShellAppend(cmdline, " 2>/dev/null", Options(options).setRedirectStderrToNull(false), log);
    }

    if (options.getRedirectStdoutToNull()) {
        return createViaShellAppend(cmdline, " 1>/dev/null", Options(options).setRedirectStdoutToNull(false), log);
    }

    // The redirection flags should have been dealt with by now.
    PRIME_ASSERT(!NeedCommandLineHack(options));

    const char* executable = "/bin/sh";
    const char* argv[4] = { "sh", "-c", cmdline, NULL };

    return create(executable, argv, options, log);
}

bool UnixProcess::createViaShellAppend(const char* cmdline, const char* append, const Options& options, Log* log)
{
    FormatBuffer<> appended("%s%s", cmdline, append);

    return createViaShell(appended.c_str(), options, log);
}

bool UnixProcess::create(const char* executable, const char* const* argv, const Options& options, Log* log)
{
    PRIME_ASSERT(_pid < 0); // Still have a process!

    UnixCloseOnExec::ScopedLock execLock; // Make sure we're the only one execing right now.

    int childStdin[2];
    int childStdout[2];

    if (options.getRead() || options.getWrite()) {
        while (pipe(childStdin) != 0) {
            if (errno != EINTR) {
                log->logErrno(errno);
                return false;
            }
        }

        while (pipe(childStdout) != 0) {
            if (errno != EINTR) {
                log->logErrno(errno);
                close(childStdin[0]);
                close(childStdin[1]);
                return false;
            }
        }
    } else {
        // Silence compiler warnings.
        childStdin[0] = childStdin[1] = childStdout[0] = childStdout[1] = -1;
    }

    if (options.getLogCommandLine()) {
        std::string commandline;
        commandline += executable;
        for (const char* const* p = argv; *p; ++p) {
            commandline += " \"";
            commandline += *p;
            commandline += "\"";
        }

        log->trace("%s", commandline.c_str());
    }

#if defined(PRIME_OS_HAS_POSIX_SPAWN)

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    if (options.getRead() || options.getWrite()) {
        posix_spawn_file_actions_addclose(&actions, childStdout[readPipe]);
        if (options.getRead()) {
            posix_spawn_file_actions_addclose(&actions, stdoutHandle);
            posix_spawn_file_actions_adddup2(&actions, childStdout[writePipe], stdoutHandle);
        } else {
            posix_spawn_file_actions_addclose(&actions, childStdout[writePipe]);
        }

        posix_spawn_file_actions_addclose(&actions, childStdin[writePipe]);
        if (options.getWrite()) {
            posix_spawn_file_actions_addclose(&actions, stdinHandle);
            posix_spawn_file_actions_adddup2(&actions, childStdin[readPipe], stdinHandle);
        } else {
            posix_spawn_file_actions_addclose(&actions, childStdin[readPipe]);
        }
    }

    if (options.getRedirectStderrToStdout()) {
        posix_spawn_file_actions_addclose(&actions, stderrHandle);
        posix_spawn_file_actions_adddup2(&actions, stdoutHandle, stderrHandle);
    }

    short spawnFlags = 0;

#ifdef CLOSE_ALL_CHILD_HANDLES
#if defined(PRIME_OS_OSX)
    spawnFlags |= POSIX_SPAWN_CLOEXEC_DEFAULT;
#else
    for (int fd = 3; fd < getdtablesize(); ++fd) {
        posix_spawn_file_actions_addclose(&actions, fd);
    }
#endif
#endif

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    posix_spawnattr_setflags(&attr, spawnFlags);

    int spawnResult = posix_spawn(&_pid, argv[0], &actions, &attr, (char* const*)argv, NULL);

    posix_spawnattr_destroy(&attr);
    posix_spawn_file_actions_destroy(&actions);

    if (spawnResult != 0) {
        log->logErrno(errno);

        if (options.getRead() || options.getWrite()) {
            for (int side = 0; side != 2; ++side) {
                close(childStdin[side]);
                close(childStdout[side]);
            }
        }

        return false;
    }

#else

    do {
        _pid = FORK_API();
    } while (_pid < 0 && errno == EINTR);

    if (_pid < 0) {
        log->logErrno(errno);

        if (options.getRead() || options.getWrite()) {
            for (int side = 0; side != 2; ++side) {
                close(childStdin[side]);
                close(childStdout[side]);
            }
        }

        return false;
    }

    if (_pid == 0) {
        // We're the child process.

        if (options.getRead() || options.getWrite()) {
            close(childStdout[readPipe]);
            if (options.getRead()) {
                close(stdoutHandle);
                PRIME_EXPECT(dup2(childStdout[writePipe], stdoutHandle) >= 0);
            } else {
                close(childStdout[writePipe]);
            }

            close(childStdin[writePipe]);
            if (options.getWrite()) {
                close(stdinHandle);
                PRIME_EXPECT(dup2(childStdin[readPipe], stdinHandle) >= 0);
            } else {
                close(childStdin[readPipe]);
            }
        }

        if (options.getRedirectStderrToStdout()) {
            close(stderrHandle);
            PRIME_EXPECT(dup2(stdoutHandle, stderrHandle) >= 0);
        }

#ifdef CLOSE_ALL_CHILD_HANDLES
        for (int fd = 3; fd < getdtablesize(); ++fd) {
            close(fd);
        }
#endif

        execvp(executable, (char* const*)argv);
        perror("execv");
        FORK_EXIT_API(1);
    }

#endif

    // We're the parent process.

    _exitCode = ExitCodeFailed;

    if (options.getRead()) {
        if (options.getWrite()) {
            _theirStdout = childStdout[readPipe];
            _theirStdin = childStdin[writePipe];
        } else {
            close(childStdin[writePipe]);
            _theirStdout = childStdout[readPipe];
            _theirStdin = -1;
        }
    } else if (options.getWrite()) {
        close(childStdout[readPipe]);
        _theirStdin = childStdin[writePipe];
        _theirStdout = -1;
    } else {
        _theirStdin = -1;
        _theirStdout = -1;
    }

    if (options.getRead() || options.getWrite()) {
        close(childStdin[readPipe]);
        close(childStdout[writePipe]);
    }

    execLock.unlock();

    if (options.getWait()) {
        int exitCode = wait(log);
        if (isSpecialExitCode(exitCode)) {
            detach();

            switch (exitCode) {
            default:
                log->error(PRIME_LOCALISE("%s: Launch failed (%d)."), executable, exitCode);
                break;

            case ExitCodeFailed:
                log->logErrno(ENOENT);
                break;
            }

            return false;
        }
    }

    return true;
}

void UnixProcess::detach()
{
    closePipes();
    _pid = -1;
}

void UnixProcess::closePipes()
{
    if (_theirStdin >= 0) {
        close(_theirStdin);
        _theirStdin = -1;
    }

    if (_theirStdout >= 0) {
        close(_theirStdout);
        _theirStdout = -1;
    }
}

bool UnixProcess::isRunning()
{
    if (_pid < 0) {
        return false;
    }

    int status;
    pid_t ret;

    do {
        ret = wait4(_pid, &status, WNOHANG, 0);
    } while (ret == -1 && errno == EINTR);

    // Trace("WIFEXITED %d WIFSIGNALED %d WIFSTOPPED %d\n", WIFEXITED(ret), WIFSIGNALED(ret), WIFSTOPPED(ret));

    if (ret == 0) {
        return true;
    }

    if (ret != -1 && WIFEXITED(status)) {
        _exitCode = WEXITSTATUS(status);
    } else {
        _exitCode = ExitCodeKilled;
    }

    closePipes();
    return false;
}

int UnixProcess::getExitCode()
{
    return _exitCode == 127 ? ExitCodeFailed : _exitCode;
}

int UnixProcess::wait(Log* log)
{
    if (_pid < 0) {
        return getExitCode();
    }

    bool pipeFailed = !endWrite(log);

    if (_theirStdout >= 0) {
        while (close(_theirStdout) != 0) {
            if (errno != EINTR) {
                log->logErrno(errno);
                pipeFailed = true;
                break;
            }
        }

        _theirStdout = -1;
    }

    int status;
    pid_t ret;

    do {
        ret = wait4(_pid, &status, 0, 0);
    } while (ret == -1 && errno == EINTR);

    // Trace("WIFEXITED %d WIFSIGNALED %d WIFSTOPPED %d\n", WIFEXITED(ret), WIFSIGNALED(ret), WIFSTOPPED(ret));

    if (ret != 0 && ret != -1) {
        if (WIFEXITED(status)) {
            _exitCode = WEXITSTATUS(status);
        } else {
            _exitCode = ExitCodeKilled;
        }
    } else {
        log->logErrno(errno);
        _exitCode = ExitCodeKilled;
    }

    if (pipeFailed) {
        _exitCode = ExitCodeKilled;
    }

    closePipes();
    return getExitCode();
}

ptrdiff_t UnixProcess::read(void* buffer, size_t bufferSize, Log* log)
{
    if (_theirStdout < 0) {
        return 0;
    }

    ptrdiff_t bytesRead;
    do {
        bytesRead = ::read(_theirStdout, buffer, bufferSize);
    } while (bytesRead < 0 && errno == EINTR);

    if (bytesRead > 0) {
        return bytesRead;
    }

    if (bytesRead == 0) {
        close(_theirStdout);
        _theirStdout = -1;
        return 0;
    }

    log->logErrno(errno);
    return -1;
}

ptrdiff_t UnixProcess::write(const void* bytes, size_t byteCount, Log* log)
{
    if (_theirStdin < 0 || byteCount == 0) {
        return 0;
    }

    ptrdiff_t bytesWritten;
    do {
        bytesWritten = ::write(_theirStdin, bytes, byteCount);
    } while (bytesWritten < 0 && errno == EINTR);

    if (bytesWritten > 0) {
        return bytesWritten;
    }

    if (bytesWritten == 0) {
        close(_theirStdin);
        _theirStdin = -1;
        return 0;
    }

    log->logErrno(errno);
    return -1;
}

bool UnixProcess::writeExact(const void* bytes, size_t byteCount, Log* log)
{
    while (byteCount) {
        ptrdiff_t wroteCount = write(bytes, byteCount, log);
        if (wroteCount <= 0) {
            return false;
        }

        bytes = (const char*)bytes + wroteCount;
        byteCount -= wroteCount;
    }

    return true;
}

bool UnixProcess::flush(Log* log)
{
    (void)log;

    return true;
}

bool UnixProcess::endWrite(Log* log)
{
    if (_pid < 0) {
        return true;
    }

    bool result = true;

    if (_theirStdin >= 0) {
        while (close(_theirStdin) != 0) {
            if (errno != EINTR) {
                log->logErrno(errno);
                result = false;
                break;
            }
        }
        _theirStdin = -1;
    }

    return result;
}

bool UnixProcess::kill(int sig, Log* log)
{
    if (_pid < 0) {
        return true;
    }

    if (::kill(_pid, sig) == 0) {
        //detach();
        log->trace("Process killed successfully.");
        return true;
    }

    log->logErrno(errno);
    return false;
}
}
