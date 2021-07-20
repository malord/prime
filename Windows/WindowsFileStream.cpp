// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsFileStream.h"
#include "../ByteOrder.h"

namespace Prime {

PRIME_DEFINE_UID_CAST(WindowsFileStream)

WindowsFileStream::WindowsFileStream()
{
    _handle = INVALID_HANDLE_VALUE;
    _shouldClose = false;
}

WindowsFileStream::~WindowsFileStream()
{
    close(Log::getNullLog());
}

bool WindowsFileStream::open(const char* path, const OpenMode& openMode, Log* log)
{
    DWORD access;
    DWORD shareMode = 0;

    if (openMode.getReadWrite()) {
        access = GENERIC_READ | GENERIC_WRITE;
    } else if (openMode.getWrite()) {
        access = GENERIC_WRITE;
    } else {
        access = GENERIC_READ;
        shareMode |= FILE_SHARE_READ;
    }

    if (openMode.getAppend()) {
        access &= ~(DWORD)FILE_WRITE_DATA;
        access |= FILE_APPEND_DATA;
    }

    DWORD creation;

    if (openMode.getCreate()) {
        if (openMode.getDoNotOverwrite()) {
            creation = CREATE_NEW;
        } else if (openMode.getTruncate()) {
            creation = CREATE_ALWAYS;
        } else {
            creation = OPEN_ALWAYS;
        }
    } else {
        if (openMode.getTruncate()) {
            creation = TRUNCATE_EXISTING;
        } else {
            creation = OPEN_EXISTING;
        }
    }

    DWORD flagsAndAttributes = 0;

    if (openMode.getBufferSequential()) {
        flagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
    }
    if (openMode.getBufferRandomAccess()) {
        flagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
    }

#ifdef PRIME_OS_XBOX
    return windowsOpen(path, access, shareMode, NULL, creation, flagsAndAttributes, log, openMode);
#else
    SECURITY_ATTRIBUTES* sa = NULL;

    SECURITY_ATTRIBUTES inherit;
    if (openMode.getChildProcessInherit()) {
        sa = &inherit;
        memset(&inherit, 0, sizeof(inherit));
        inherit.nLength = sizeof(inherit);
        inherit.bInheritHandle = TRUE;
    }

    return windowsOpen(path, access, shareMode, sa, creation, flagsAndAttributes, log, openMode);
#endif
}

bool WindowsFileStream::windowsOpen(const char* path, DWORD access, DWORD shareMode, LPSECURITY_ATTRIBUTES sa, DWORD creationDisposition, DWORD flagsAndAttributes, Log* log, const OpenMode& openMode)
{
    (void)openMode;

    close(log);

    HANDLE createdHandle = CreateFile(CharToTChar(path).c_str(), access, shareMode, sa, creationDisposition, flagsAndAttributes, 0);
    if (createdHandle == INVALID_HANDLE_VALUE) {
        log->logWindowsError(GetLastError());
        return false;
    }

    attach(createdHandle, true);
    return true;
}

void WindowsFileStream::attach(HANDLE existingHandle, bool closeWhenDone)
{
    close(Log::getNullLog());

    _handle = existingHandle;
    _shouldClose = closeWhenDone;
}

HANDLE WindowsFileStream::detach()
{
    HANDLE detached = _handle;

    _handle = INVALID_HANDLE_VALUE;
    _shouldClose = false;

    return detached;
}

bool WindowsFileStream::close(Log* log)
{
    bool result = true;

    if (_shouldClose) {
        if (!CloseHandle(_handle)) {
            log->logWindowsError(GetLastError());
            result = false;
        }

        _shouldClose = false;
    }

    _handle = INVALID_HANDLE_VALUE;
    return result;
}

ptrdiff_t WindowsFileStream::readSome(void* buffer, size_t maxBytes, Log* log)
{
    PRIME_ASSERT(isOpen());
    PRIME_ASSERT((DWORD)maxBytes == maxBytes); // Truncation?

    DWORD bytesRead;
    if (!ReadFile(_handle, buffer, (DWORD)maxBytes, &bytesRead, 0)) {
        log->logWindowsError(GetLastError());
        return -1;
    }

    return (ptrdiff_t)bytesRead;
}

ptrdiff_t WindowsFileStream::writeSome(const void* bytes, size_t maxBytes, Log* log)
{
    PRIME_ASSERT(isOpen());
    PRIME_ASSERT((DWORD)maxBytes == maxBytes); // Truncation?

    DWORD bytesWritten;
    if (!WriteFile(_handle, bytes, (DWORD)maxBytes, &bytesWritten, 0)) {
        log->logWindowsError(GetLastError());
        return -1;
    }

    return (ptrdiff_t)bytesWritten;
}

Stream::Offset WindowsFileStream::seek(Offset offset, SeekMode mode, Log* log)
{
    PRIME_ASSERT(isOpen());

    DWORD method;

    switch (mode) {
    case SeekModeAbsolute:
        method = FILE_BEGIN;
        break;
    case SeekModeRelative:
        method = FILE_CURRENT;
        break;
    case SeekModeRelativeToEnd:
        method = FILE_END;
        break;
    default:
        PRIME_ASSERT(0);
        return -1;
    }

    DWORD low = Low32(offset);
    LONG high = (LONG)High32(offset);

    SetLastError(0);

    low = SetFilePointer(_handle, low, &high, method);

    if (low == INVALID_SET_FILE_POINTER) {
        DWORD winerr = GetLastError();

        if (winerr != NO_ERROR) {
            log->logWindowsError(winerr);
            return -1;
        }
    }

    return (int64_t)Make64(low, high);
}

Stream::Offset WindowsFileStream::getSize(Log* log)
{
    PRIME_ASSERT(isOpen());

    SetLastError(0);

    DWORD high;
    DWORD low = GetFileSize(_handle, &high);

    if (low == INVALID_FILE_SIZE) {
        DWORD winerr = GetLastError();
        if (winerr != NO_ERROR) {
            log->logWindowsError(winerr);
            return -1;
        }
    }

    return Make64(low, high);
}

bool WindowsFileStream::setSize(Offset newSize, Log* log)
{
    PRIME_ASSERT(isOpen());

    int64_t back = getOffset(log);
    if (back < 0) {
        return false;
    }

    if (!setOffset(newSize, log)) {
        return false;
    }

    BOOL success = SetEndOfFile(_handle);
    DWORD winerr = GetLastError();

    if (!setOffset(back, log)) {
        return false;
    }

    if (!success) {
        log->logWindowsError(winerr);
        return false;
    }

    return true;
}

bool WindowsFileStream::flush(Log*)
{
    return true;
}
}
