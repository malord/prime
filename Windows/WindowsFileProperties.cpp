// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsFileProperties.h"

namespace Prime {

WindowsFileProperties::WindowsFileProperties()
{
}

WindowsFileProperties::~WindowsFileProperties()
{
}

bool WindowsFileProperties::read(const char* path, Log* log)
{
    if (!_dir.openWildcard(path, log)) {
        return false;
    }

    if (!_dir.read(log)) {
        return false;
    }

    _dir.close();
    return true;
}

bool WindowsFileProperties::readTChar(LPCTSTR path, Log* log)
{
    if (!_dir.openTCharWildcard(path, log)) {
        return false;
    }

    if (!_dir.read(log)) {
        return false;
    }

    _dir.close();
    return true;
}

bool WindowsFileProperties::applyTimes(const char* path, Log* log) const
{
    HANDLE handle = CreateFile(CharToTChar(path).c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        log->logWindowsError(GetLastError());
        return false;
    }

    if (!SetFileTime(handle, &_dir->ftCreationTime, &_dir->ftLastAccessTime, &_dir->ftLastWriteTime)) {
        log->logWindowsError(GetLastError());
        return false;
    }

    CloseHandle(handle);
    return true;
}

void WindowsFileProperties::setCreationTime(const UnixTime& unixTime)
{
    unixTime.toWindowsFileTime(_dir->ftCreationTime);
}

void WindowsFileProperties::setLastAccessTime(const UnixTime& unixTime)
{
    unixTime.toWindowsFileTime(_dir->ftLastAccessTime);
}

void WindowsFileProperties::setModificationTime(const UnixTime& unixTime)
{
    unixTime.toWindowsFileTime(_dir->ftLastWriteTime);
}

bool WindowsFileProperties::applyWindowsAttributes(const char* path, Log* log) const
{
    const uint32_t attributesToChange = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN;

    uint32_t newAttributes = _dir->dwFileAttributes & attributesToChange;

    WindowsFileProperties pathProps;
    if (pathProps.read(path, Log::getNullLog())) {
        newAttributes = (pathProps.getWindowsAttributes() & (~attributesToChange)) | newAttributes;
    }

    if (::SetFileAttributes(CharToTChar(path).c_str(), newAttributes)) {
        return true;
    }

    log->logWindowsError(GetLastError());
    return false;
}
}
