// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsDirectoryReader.h"
#include "../Path.h"

namespace Prime {

WindowsDirectoryReader::WindowsDirectoryReader()
{
    _handle = INVALID_HANDLE_VALUE;
    _readHasBeenCalled = false;
    _first = false;
}

WindowsDirectoryReader::~WindowsDirectoryReader()
{
    close();
}

WindowsDirectoryReader& WindowsDirectoryReader::operator=(const WindowsDirectoryReader& copy)
{
    if (this != &copy) {
        close();
        _data = copy._data;
        _first = copy._first;
        _readHasBeenCalled = copy._readHasBeenCalled;
        if (_readHasBeenCalled) {
            _filename = TCharToChar(_data.cFileName);
        }
    }

    return *this;
}

bool WindowsDirectoryReader::open(const char* path, Log* log, const Options& options)
{
    std::string wildcard = Path::join(path, "*");
    return openWildcard(wildcard.c_str(), log, options);
}

bool WindowsDirectoryReader::openWildcard(const char* wildcard, Log* log, const Options& options)
{
    return openTCharWildcard(CharToTChar(wildcard).c_str(), log, options);
}

bool WindowsDirectoryReader::openTCharWildcard(LPCTSTR wildcard, Log* log, const Options&)
{
    close();

    _handle = FindFirstFile(wildcard, &_data);
    if (_handle == INVALID_HANDLE_VALUE) {
        DWORD winerr = GetLastError();
        if (winerr != ERROR_NO_MORE_FILES) {
            log->logWindowsError(winerr);
            return false;
        }
    }

    // Windows does something daft with "." and ".." - data.cFileName becomes the name of the directory, so if
    // you're in C:\\WINDOWS then FindFirstFile(".") gives you "WINDOWS" as the file name. There's probably some
    // completely logical Raymond Chen explanation for this but it still sucks.
    if (isDotFilename(wildcard)) {
        lstrcpy(_data.cFileName, TEXT("."));
    } else if (isDotDotFilename(wildcard)) {
        lstrcpy(_data.cFileName, TEXT(".."));
    }

    _first = true;
    return true;
}

void WindowsDirectoryReader::close()
{
    if (_handle != INVALID_HANDLE_VALUE) {
        FindClose(_handle);
        _handle = INVALID_HANDLE_VALUE;
    }

    _first = false;
}

bool WindowsDirectoryReader::read(Log* log, bool* error)
{
    _readHasBeenCalled = false;

    if (error) {
        *error = false;
    }

    if (_handle == INVALID_HANDLE_VALUE) {
        _first = false;
        return false;
    }

    if (_first || FindNextFile(_handle, &_data)) {
        _first = false;
        _readHasBeenCalled = true;
        _filename = TCharToChar(_data.cFileName);
        return true;
    }

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        log->logWindowsError(GetLastError(), "FindNextFile");
        if (error) {
            *error = true;
        }
    }

    close();
    return false;
}

bool WindowsDirectoryReader::isDotFilename(LPCTSTR path)
{
    return lstrcmp(findFilename(path), TEXT(".")) == 0;
}

bool WindowsDirectoryReader::isDotDotFilename(LPCTSTR path)
{
    return lstrcmp(findFilename(path), TEXT("..")) == 0;
}

LPCTSTR WindowsDirectoryReader::findFilename(LPCTSTR path)
{
    LPCTSTR at = path + lstrlen(path);
    while (at-- != path) {
        if (*at < SCHAR_MAX && Path::isSeparator((char)*at)) {
            break;
        }
    }

    return at + 1;
}
}
