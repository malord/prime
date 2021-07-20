// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsProcess.h"
#include "../Mutex.h"

namespace {

using namespace Prime;

static Mutex* OneLaunchAtATimeMutex()
{
    static Mutex mutex(Log::getGlobal());
    return &mutex;
}

static bool ProcessOptionsRequireStdHandles(const WindowsProcess::Options& options)
{
    return options.getRead() || options.getWrite() || options.getRedirectStderrToStdout() || options.getRedirectStderrToNull() || options.getRedirectStdoutToNull();
}

bool DoesArgNeedEscaping(const char* arg)
{
    const char* ptr;

    for (ptr = arg; *ptr; ++ptr) {
        if (*ptr == ' ' || *ptr == '\"' || *ptr == '\t') {
            return true;
        }
    }

    return false;
}

void AppendQuotedArgument(std::string& buffer, const char* arg)
{
    buffer += '"';

    const char* last = arg;
    for (const char* ptr = arg;; ++ptr) {
        if (!*ptr || *ptr == '"') {
            const char* backslashes;

            buffer.append(last, ptr - last);

            // Backslashes before a double quote (including the double quote at the end of the argument) need to
            // be escaped.
            backslashes = ptr;
            while (backslashes-- != arg && *backslashes == '\\') { }

            ++backslashes;
            buffer.append(backslashes, ptr - backslashes);

            // End of string?
            if (!*ptr) {
                break;
            }

            PRIME_ASSERT(*ptr == '"');

            buffer.append("\"\"", 2);

            last = ptr + 1;
        }
    }

    buffer += '"';
}

void BuildCommandLine(std::string& cmdline, const WindowsProcess::Argument* args, size_t nargs)
{
    for (size_t i = 0; i != nargs; ++i) {
        const WindowsProcess::Argument* arg = &args[i];

        if (i != 0) {
            cmdline += ' ';
        }

        if (arg->isVerbatim() || !DoesArgNeedEscaping(arg->getArgument().c_str())) {
            cmdline.append(arg->getArgument());
            continue;
        }

        if (arg->getArgument().empty()) {
            // empty argument
            cmdline.append("\"\"", 2);
            continue;
        }

        AppendQuotedArgument(cmdline, arg->getArgument().c_str());
    }
}

#if _WIN32_WINNT >= 0x500
void EnableInheritHandle(HANDLE handle)
{
    if (handle && handle != INVALID_HANDLE_VALUE) {
        SetHandleInformation(handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    }
}
#endif

bool CloseHandleIfNotNull(HANDLE handle, Log* log)
{
    if (handle && !CloseHandle(handle)) {
        log->logWindowsError(GetLastError());
        return false;
    }

    return true;
}

bool CloseHandleIfNotInvalid(HANDLE handle, Log* log)
{
    if (handle != INVALID_HANDLE_VALUE && !CloseHandle(handle)) {
        log->logWindowsError(GetLastError());
        return false;
    }

    return true;
}

bool CreatePipePair(HANDLE* read, HANDLE* write, bool inheritRead, Log* log)
{
    *read = 0;
    *write = 0;

    // Set up security attributes to allow handles to be inheritable
    SECURITY_ATTRIBUTES secattr;
    secattr.nLength = sizeof(secattr);
    secattr.bInheritHandle = true;
    secattr.lpSecurityDescriptor = 0;

    // Create an anonymous pipe
    if (!CreatePipe(read, write, &secattr, 0)) {
        log->logWindowsError(GetLastError());
        return false;
    }

    // Create the handle for the other process
    HANDLE dup = 0;
    HANDLE* noinherit = inheritRead ? write : read;
    if (!DuplicateHandle(GetCurrentProcess(), *noinherit, GetCurrentProcess(), &dup, 0, false, DUPLICATE_SAME_ACCESS)) {
        log->logWindowsError(GetLastError());
        return false;
    }

    CloseHandle(*noinherit);
    *noinherit = dup;
    return true;
}
}

namespace Prime {

WindowsProcess::WindowsProcess()
{
    OneLaunchAtATimeMutex();
    memset(&_pipes, 0, sizeof(_pipes));
    memset(&_processInfo, 0, sizeof(_processInfo));
    _exitCode = ExitCodeFailed;
}

WindowsProcess::~WindowsProcess()
{
    if (_processInfo.hProcess) {
        wait(Log::getNullLog());
    }

    detach();
}

bool WindowsProcess::create(const Argument* args, size_t nargs, const Options& options, Log* log)
{
    PRIME_ASSERT(!_processInfo.hProcess); // already attached to a process!

    std::string cmdline;
    BuildCommandLine(cmdline, args, nargs);

    return createWithCommandLine(cmdline.c_str(), options, log);
}

bool WindowsProcess::createWithCommandLine(const char* cmdline, const Options& options, Log* log)
{
    Mutex::ScopedLock oneLaunchAtATime(OneLaunchAtATimeMutex());

    if (options.getLogCommandLine()) {
        log->trace("%s", cmdline);
    }

    if (options.getRead() || options.getWrite()) {
        if (!CreatePipePair(&_pipes.outRead, &_pipes.outWrite, false, log) || !CreatePipePair(&_pipes.inRead, &_pipes.inWrite, true, log)) {
            closePipes(Log::getNullLog());
            return false;
        }
    }

    HANDLE nul = INVALID_HANDLE_VALUE;
    if (options.getRedirectStdoutToNull() || options.getRedirectStderrToNull()) {
        SECURITY_ATTRIBUTES inherit;
        memset(&inherit, 0, sizeof(inherit));
        inherit.nLength = sizeof(inherit);
        inherit.bInheritHandle = true;

        nul = CreateFile(TEXT("nul"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &inherit, OPEN_EXISTING, 0, 0);
    }

    STARTUPINFO startupInfo;
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    if (ProcessOptionsRequireStdHandles(options)) {
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;
        startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

// It's rare that the standard handles are non-inheritable so the fact this doesn't have a Win98
// equivalent isn't a worry.
#if _WIN32_WINNT >= 0x500
        EnableInheritHandle(GetStdHandle(STD_INPUT_HANDLE));
        EnableInheritHandle(GetStdHandle(STD_OUTPUT_HANDLE));
        EnableInheritHandle(GetStdHandle(STD_ERROR_HANDLE));
#endif

        if (options.getRedirectStdoutToNull()) {
            startupInfo.hStdOutput = nul;
        }

        if (options.getRedirectStderrToStdout()) {
            startupInfo.hStdError = startupInfo.hStdOutput;
        } else if (options.getRedirectStderrToNull()) {
            startupInfo.hStdError = nul;
        }

        if (options.getRead()) {
            startupInfo.hStdOutput = _pipes.outWrite;
        }

        if (options.getWrite()) {
            startupInfo.hStdInput = _pipes.inRead;
        }
    }

    DWORD creationFlags = 0;

    if (!options.getShowConsole()) {
        creationFlags |= CREATE_NO_WINDOW;
    }

    // CreateProcess may overwrite the string (!). CharToTChar returns a const TCHAR*, so I just cast it since
    // it's not used again.
    if (!CreateProcess(0, const_cast<TCHAR*>(CharToTChar(cmdline).c_str()), 0, 0, true, creationFlags, 0, 0, &startupInfo, &_processInfo)) {
        log->logWindowsError(GetLastError());
        CloseHandleIfNotInvalid(nul, Log::getNullLog());
        closePipes(Log::getNullLog());
        memset(&_processInfo, 0, sizeof(_processInfo));
        return false;
    }

    // Put something in exitCode in case the caller calls getExitCode() without waiting for the process to exit.
    _exitCode = ExitCodeFailed;

    if (options.getRead() || options.getWrite()) {
        if (!options.getWrite()) {
            endWrite(log); // closes _pipes.inWrite
        }

        if (!options.getRead()) {
            CloseHandle(_pipes.outRead);
            _pipes.outRead = 0;
        }

        CloseHandle(_pipes.outWrite);
        _pipes.outWrite = 0;

        CloseHandle(_pipes.inRead);
        _pipes.inRead = 0;
    }

    oneLaunchAtATime.unlock();

    CloseHandleIfNotInvalid(nul, log);

    if (options.getWait()) {
        int exitCode = wait(log);
        if (isSpecialExitCode(exitCode)) {
            detach();
            return false;
        }
    }

    return true;
}

void WindowsProcess::detach()
{
    closeEverything(Log::getNullLog());
}

bool WindowsProcess::waitTimeout(DWORD timeout, Log* log)
{
    // Already exited?
    if (!_processInfo.hProcess) {
        return true;
    }

    if (WaitForSingleObject(_processInfo.hProcess, timeout) != WAIT_OBJECT_0) {
        return false;
    }

    DWORD windowsExitCode;
    if (!GetExitCodeProcess(_processInfo.hProcess, &windowsExitCode) || windowsExitCode == STILL_ACTIVE) {
        _exitCode = ExitCodeKilled;
    } else {
        _exitCode = (int)windowsExitCode;
    }

    closeEverything(log);
    return true;
}

int WindowsProcess::wait(Log* log)
{
    closePipes(log);
    waitTimeout(INFINITE, log);
    return _exitCode;
}

void WindowsProcess::closeEverything(Log* log)
{
    CloseHandleIfNotNull(_processInfo.hProcess, log);
    CloseHandleIfNotNull(_processInfo.hThread, log);

    memset(&_processInfo, 0, sizeof(_processInfo));

    closePipes(log);
}

bool WindowsProcess::closePipes(Log* log)
{
    bool success = true;

    success = CloseHandleIfNotNull(_pipes.outWrite, log) && success;
    success = CloseHandleIfNotNull(_pipes.outRead, log) && success;
    success = CloseHandleIfNotNull(_pipes.inWrite, log) && success;
    success = CloseHandleIfNotNull(_pipes.inRead, log) && success;

    memset(&_pipes, 0, sizeof(_pipes));
    return success;
}

bool WindowsProcess::isRunning()
{
    return !waitTimeout(0, Log::getNullLog());
}

bool WindowsProcess::endWrite(Log* log)
{
    bool result = true;

    if (_pipes.inWrite) {
        if (!CloseHandle(_pipes.inWrite)) {
            log->logWindowsError(GetLastError());
            result = false;
        }

        _pipes.inWrite = 0;
    }

    return result;
}

ptrdiff_t WindowsProcess::read(void* buffer, size_t bufferSize, Log* log)
{
    if (!_pipes.outRead) {
        return 0;
    }

    DWORD bytesRead;
    if (!ReadFile(_pipes.outRead, buffer, (DWORD)bufferSize, &bytesRead, 0)) {
        DWORD err = GetLastError();

        // Other end of pipe closed?
        if (err == ERROR_BROKEN_PIPE) {
            return 0;
        }

        log->logWindowsError(GetLastError());
        return -1;
    }

    return (ptrdiff_t)bytesRead;
}

ptrdiff_t WindowsProcess::write(const void* bytes, size_t byteCount, Log* log)
{
    if (!_pipes.inWrite) {
        return 0;
    }

    DWORD bytesWritten;
    if (!WriteFile(_pipes.inWrite, bytes, (DWORD)byteCount, &bytesWritten, 0)) {
        log->logWindowsError(GetLastError());
        return 0;
    }

    return (ptrdiff_t)bytesWritten;
}

bool WindowsProcess::writeExact(const void* bytes, size_t byteCount, Log* log)
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

bool WindowsProcess::flush(Log* log)
{
    if (!_pipes.inWrite) {
        return true;
    }

    if (!FlushFileBuffers(_pipes.inWrite)) {
        log->logWindowsError(GetLastError());
        return false;
    }

    return true;
}

}
