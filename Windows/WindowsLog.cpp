// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsLog.h"
#include "../ANSIEscapeParser.h"
#include "WindowsConfig.h"
#include <io.h>
#include <stdio.h>

namespace {

using namespace Prime;

// _fputws is really really slow at writing to the console, so use WriteConsole if the stream
// (stdout/stderr) is attached to the console. Since WriteConsole allows me to use colours,
// I felt compelled to implement ANSI colour escapes. You may think this is a ridiculous amount
// of work for something so trivial, and here I am just after writing it, agreeing.

class WindowsANSIConsole {
public:
    WindowsANSIConsole();

    /// Initialise when the console handle is not known. Fails if neither stdout nor stderr are consoles.
    bool init();

    /// Initialise given either stdout or stderr. Returns false if the stream is not a TTY.
    bool init(FILE* stream);

    bool init(HANDLE consoleHandle);

    bool isInitialised() const { return _initialised; }

    void write(HANDLE consoleHandle, StringView string);

    bool doesTerminalHaveDarkBackground() const { return !isColourBright[_defaultBackground]; }

private:
    CRITICAL_SECTION _criticalSection;
    bool _initialised;
    WORD _defaultForeground, _defaultBackground, _defaultAttributes;
    ANSIEscapeParser _ansi;

    static const bool isColourBright[16];
};

const bool WindowsANSIConsole::isColourBright[16] = {
    false, // Black
    false, // Blue
    false, // Green
    false, // Cyan
    false, // Red
    false, // Magenta
    false, // Yellow
    true, // Grey
    false, // Dark Grey
    false, // Bright Blue
    true, // Bright Green
    true, // Bright Cyan
    false, // Bright Red
    false, // Bright Magenta
    true, // Bright Yellow
    true, // White
};

WindowsANSIConsole::WindowsANSIConsole()
    : _initialised(false)
{
}

bool WindowsANSIConsole::init()
{
    return init(stdout) || init(stderr);
}

bool WindowsANSIConsole::init(FILE* stream)
{
    if (!_isatty(fileno(stream))) {
        return false;
    }

    HANDLE consoleHandle = (HANDLE)_get_osfhandle(fileno(stream));
    return init(consoleHandle);
}

bool WindowsANSIConsole::init(HANDLE consoleHandle)
{
    if (_initialised) {
        return true;
    }

    InitializeCriticalSection(&_criticalSection);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    memset(&csbi, 0, sizeof(csbi));
    if (GetConsoleScreenBufferInfo(consoleHandle, &csbi)) {
        _defaultForeground = (WORD)(csbi.wAttributes & 0x0f);
        _defaultBackground = (WORD)((csbi.wAttributes >> 4) & 0x0f);
        _defaultAttributes = csbi.wAttributes;
    } else {
        _defaultForeground = 7;
        _defaultBackground = 0;
        _defaultAttributes = 7;
    }

    _initialised = true;
    return true;
}

void WindowsANSIConsole::write(HANDLE consoleHandle, StringView string)
{
    // Use a critical section to stop multiple threads trampling over each other's output.
    init(consoleHandle);

    EnterCriticalSection(&_criticalSection);

    const char* ptr = string.begin();
    const char* stringEnd = string.end();

    for (;;) {

        ptr = _ansi.process(ptr, stringEnd);

        // Now grab as much of the text as we can store in a buffer, up to a newline or escape character.
        char buffer[128];
        const char* start = ptr;
        while (ptr != stringEnd && *ptr != '\n' && *ptr != 27 && (size_t)(ptr - start) < sizeof(buffer) - 1) {
            ++ptr;
        }

        // Write the buffer to the log, with the correct attributes.
        if (ptr != start) {
            memcpy(buffer, start, ptr - start);
            buffer[ptr - start] = 0;

            static const BYTE ansiColourToDOS[16] = { 0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15 };

            WORD fg = _ansi.foreground < 0 ? _defaultForeground : ansiColourToDOS[_ansi.foreground];
            WORD bg = _ansi.background < 0 ? _defaultBackground : ansiColourToDOS[_ansi.background];

            // Check to make sure the foreground and background colours haven't ended up identical.
            if (fg == bg) {
                fg ^= 0x0f;
            }

            WORD attributes = (WORD)((_defaultAttributes & 0xff00) | fg | (bg << 4));

            TCharString converted = CharToTChar(buffer);

            SetConsoleTextAttribute(consoleHandle, attributes);

            DWORD numberWritten = 0;
            WriteConsole(consoleHandle, converted.c_str(), lstrlen(converted.c_str()), &numberWritten, NULL);

            SetConsoleTextAttribute(consoleHandle, _defaultAttributes);
        }

        if (ptr == stringEnd) {
            break;
        }

        if (*ptr == '\n') {
            DWORD numberWritten = 0;
            TCHAR newline = '\n';
            WriteConsole(consoleHandle, &newline, 1, &numberWritten, NULL);
        }

        if (*ptr != 27) {
            ++ptr;
        }
    }

    LeaveCriticalSection(&_criticalSection);
}

WindowsANSIConsole* GetWindowsANSIConsole()
{
    static WindowsANSIConsole console;
    return &console;
}
}

namespace Prime {

WindowsLog::WindowsLog()
{
    consoleChanged();
}

void WindowsLog::consoleChanged()
{
    _consoleAttached = GetConsoleCP() != 0;
}

void WindowsLog::write(Level level, const char* string)
{
    if (getOnlyUseOutputDebugString() || !_consoleAttached) {
        OutputDebugString(CharToTChar(string).c_str());
        return;
    }

    FILE* stream = getStreamForLevel(level);

    // TODO: cache result of _isatty?
    if (_isatty(fileno(stream))) {
        HANDLE consoleHandle = (HANDLE)_get_osfhandle(fileno(stream));
        GetWindowsANSIConsole()->write(consoleHandle, string);
        return;
    }

    if (stream != stdout) {
        fflush(stdout);
    }

#ifdef PRIME_OS_WINDOWS_UNICODE
    fputws(CharToTChar(string).c_str(), stream);
#else
    fputs(string, stream);
#endif
}

FILE* WindowsLog::getStreamForLevel(Level level) const
{
    return getUseStdoutForLevel(level) ? stdout : stderr;
}

bool WindowsLog::isColourSupportedForLevel(Level level) const
{
    return isOutputATTYForLevel(level);
}

bool WindowsLog::isOutputATTYForLevel(Level level) const
{
    bool value;
    if (!getCachedIsATTYForLevel(level, value)) {
        if (getOnlyUseOutputDebugString() || !_consoleAttached) {
            value = false;
        } else {
            value = _isatty(fileno(getStreamForLevel(level))) ? true : false;
        }

        setCachedIsATTYForLevel(level, value);
    }

    return value;
}

bool WindowsLog::doesTerminalHaveDarkBackground() const
{
    GetWindowsANSIConsole()->init();
    return GetWindowsANSIConsole()->doesTerminalHaveDarkBackground();
}
}
