// Copyright 2000-2021 Mark H. P. Lord

#include "../File.h"
#include "../Path.h"
#include "../ScopedPtr.h"
#include "WindowsConfig.h"
#include <Shlwapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "shlwapi.lib")
#endif

namespace Prime {

namespace {

    bool HasDirectoryAttribute(DWORD attributes)
    {
        return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    bool HasHiddenAttributes(DWORD attributes)
    {
        return (attributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) != 0;
    }
}

bool GetWindowsFileAttributes(uint32_t& attributes, const char* path, Log* log)
{
    WIN32_FIND_DATA findData;
    HANDLE handle;

    handle = FindFirstFile(CharToTChar(path).c_str(), &findData);

    if (handle == INVALID_HANDLE_VALUE) {
        log->logWindowsError(GetLastError());
        return false;
    }

    FindClose(handle);
    attributes = findData.dwFileAttributes;
    return true;
}

bool SetWindowsFileAttributes(const char* path, uint32_t attributes, Log* log)
{
    if (::SetFileAttributes(CharToTChar(path).c_str(), attributes)) {
        return true;
    }

    log->logWindowsError(GetLastError());
    return false;
}

bool ClearHiddenSystemReadOnlyAttributes(const char* path, Log* log)
{
    uint32_t attributes;
    if (!GetWindowsFileAttributes(attributes, path, log)) {
        return false;
    }

    uint32_t newAttributes = attributes & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY);

    if (attributes == newAttributes) {
        return true;
    }

    return SetWindowsFileAttributes(path, newAttributes, log);
}

bool FileExists(const char* path, Log* log)
{
    WIN32_FIND_DATA findData;
    HANDLE handle;

    handle = FindFirstFile(CharToTChar(path).c_str(), &findData);

    if (handle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND) {
            return false;
        }

        log->logWindowsError(error);
        return false;
    }

    FindClose(handle);
    return true;
}

bool RenameFile(const char* from, const char* to, Log* log)
{
    BOOL result = MoveFile(CharToTChar(from).c_str(), CharToTChar(to).c_str());

    if (!result) {
        log->logWindowsError(GetLastError());
    }

    return result ? true : false;
}

bool RenameFileOverwrite(const char* from, const char* to, Log* log)
{
#if WINVER >= PRIME_WINVER_FOR(PRIME_WINDOWS_XP)
    BOOL result = MoveFileEx(CharToTChar(from).c_str(), CharToTChar(to).c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);

    if (!result) {
        log->logWindowsError(GetLastError());
    }

    return result ? true : false;
#else
    if (FileExists(to, log)) {
        RemoveFile(to, log);
    }

    return RenameFile(from, to, log);
#endif
}

bool RemoveFile(const char* path, Log* log)
{
    BOOL result = DeleteFile(CharToTChar(path).c_str());

    if (!result) {
        log->logWindowsError(GetLastError());
    }

    return result ? true : false;
}

bool MakeDirectory(const char* path, Log* log, unsigned int permissions)
{
    (void)permissions;

    BOOL result = CreateDirectory(CharToTChar(path).c_str(), 0);

    if (!result) {
        log->logWindowsError(GetLastError());
    }

    return result ? true : false;
}

bool RemoveEmptyDirectory(const char* path, Log* log)
{
    BOOL result = RemoveDirectory(CharToTChar(path).c_str());

    if (!result) {
        log->logWindowsError(GetLastError());
    }

    return result ? true : false;
}

#ifndef PRIME_OS_XBOX
bool GetWorkingDirectory(std::string& out, Log* log)
{
    TCHAR* buffer = NULL;
    size_t bufferSize = MAX_PATH;

    for (;;) {
        if (buffer) {
            delete[] buffer;
        }

        buffer = new TCHAR[bufferSize];

        DWORD size = GetCurrentDirectory((DWORD)bufferSize, buffer);

        if (!size) {
            log->logWindowsError(GetLastError());
            delete[] buffer;
            return false;
        }

        if (size < bufferSize) {
            out = TCharToChar(buffer);
            delete[] buffer;
            return true;
        }

        delete[] buffer;
        bufferSize *= 2;

        if (bufferSize >= 32768) {
            log->logWindowsError(ERROR_FILE_NOT_FOUND); // making up error code
            return false;
        }
    }
}

bool SetWorkingDirectory(const char* path, Log* log)
{
    if (!SetCurrentDirectory(CharToTChar(path).c_str())) {
        log->logWindowsError(GetLastError());
        return false;
    }

    return true;
}
#endif

bool FilenameMatch(const char* pattern, const char* string, bool)
{
#ifdef PRIME_OS_XBOX
    // Can do better than this - need to ignore duplicate/wrong slashes, for example.
    return WildcardMatch(pattern, string, true, "\\/:");
#else
    return PathMatchSpec(CharToTChar(string).c_str(), CharToTChar(pattern).c_str()) ? true : false;
#endif
}

// Private functions for use by NormalisePath
namespace {

    using namespace Prime;

    inline int lstrncmp(const char* a, const char* b, size_t n)
    {
        return strncmp(a, b, n);
    }

    inline int lstrncmp(const wchar_t* a, const wchar_t* b, size_t n)
    {
        return wcsncmp(a, b, n);
    }

    bool GetWindowsFullPathName(const TCHAR* shortPath, TCharString* fullPath)
    {
        size_t bufferSize = 512;
        for (;;) {
            ScopedArrayPtr<TCHAR> buffer(new TCHAR[bufferSize]);
            DWORD result = GetFullPathName(shortPath, (DWORD)bufferSize, buffer.get(), NULL);

            if (result == 0) {
                return false;
            }

            if (result < bufferSize) {
                *fullPath = buffer.get();
                return true;
            }

            bufferSize *= 2;
            if (bufferSize >= 32768) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return false;
            }
        }
    }

    // This doesn't seem to help very much and it somtimes adds a "\\?\" when you don't want one.
    //#define USE_GETLONGPATHNAME

#ifdef USE_GETLONGPATHNAME
    bool GetWindowsLongPathName(const TCHAR* shortPath, TCharString* longPath)
    {
        size_t bufferSize = 512;
        for (;;) {
            ScopedArrayPtr<TCHAR> buffer(new TCHAR[bufferSize]);
            DWORD result = GetLongPathName(shortPath, buffer.get(), (DWORD)bufferSize);

            if (result == 0) {
                return false;
            }

            if (result < bufferSize) {
                *longPath = buffer.get();
                return true;
            }

            bufferSize *= 2;
            if (bufferSize >= 32768) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return false;
            }
        }
    }
#endif

    inline const TCHAR* SkipSlashes(const TCHAR* ptr)
    {
        while (Path::isSlash(*ptr)) {
            ++ptr;
        }

        return ptr;
    }

    /// First converts the root part of the path:
    /// - "c:" => "C:" (drive letters are uppercased)
    /// - "c:\" => "C:\"
    /// - "gAme:" => "game:" (mount point names are lowercased)
    /// - "gAme:\" => "game:\"
    /// - "\\seRVer\shaRE" => "\\SERVER\SHARE\" (there's always a backslash)
    /// - "\\seRVer\shaRE\" => "\\SERVER\SHARE\"
    ///
    /// Then processes each component of the path and calls FindFirstFile on each one to get the canonical form
    /// (i.e., the long name in the case that appears in a directory listing). Removes duplicate slashes and
    /// converts UNIX slashes to Windows slashes.
    ///
    /// This function has limited ability to deal with "." and ".." in paths. For best results, call
    /// GetFullPathName on the path first.
    TCharString CanonicaliseWindowsPath(const TCHAR* input)
    {
        TCharString output;

        // TODO: AAAGH.. "\\?\UNC\"

        // Preserve a leading "\\?\" and skip any extraneous slashes.
        if (Path::isSlash(input[0]) && Path::isSlash(input[1]) && input[2] == '?' && Path::isSlash(input[3])) {
            input = SkipSlashes(input + 4);
            output += TEXT("\\\\?\\");
        }

        // Is there a drive letter/name?
        const TCHAR* driveEnd = input;
        while (*driveEnd && !Path::isSeparator(*driveEnd)) {
            ++driveEnd;
        }
        if (*driveEnd == ':') {
            ++driveEnd; // Include the :

            if (driveEnd == input + 2) {
                // Convert drive name to uppercase.
                output += (TCHAR)(size_t)CharUpper((LPTSTR)*input);
                output += ':';
                input = driveEnd;
            } else {
                // Convert mount point names to lowercase.
                for (; input != driveEnd; ++input) {
                    output += ((TCHAR)(size_t)CharLower((LPTSTR)*input));
                }
            }

            // Add the slash, if there was one.
            if (Path::isSlash(*input)) {
                output += '\\';
                input = SkipSlashes(input + 1);
            }
        } else {
            // Is there a UNC (\\server\share\...)?
            if (Path::isSlash(input[0]) && Path::isSlash(input[1])) {
                output += TEXT("\\\\");
                input = SkipSlashes(input + 2);

                // Skip the server part
                const TCHAR* serverNameEnd = input;
                while (*serverNameEnd && !Path::isSlash(*serverNameEnd)) {
                    ++serverNameEnd;
                }

                for (; input != serverNameEnd; ++input) {
                    output += ((TCHAR)(size_t)CharUpper((LPTSTR)*input));
                }

                input = SkipSlashes(input);
                output += '\\';

                // Skip the share name. There's probably a way to get a canonical name for a share but uppercasing
                // will work fine for the purposes of comparing file names.
                if (*input) {
                    const TCHAR* shareNameEnd = input;
                    while (*shareNameEnd && !Path::isSlash(*shareNameEnd)) {
                        ++shareNameEnd;
                    }

                    for (; input != shareNameEnd; ++input) {
                        output += ((TCHAR)(size_t)CharUpper((LPTSTR)*input));
                    }

                    input = SkipSlashes(input);
                    output += '\\';
                }
            } else {
                // Do we just have a slash?
                if (Path::isSlash(*input)) {
                    output += '\\';
                    input = SkipSlashes(input + 1);
                }
            }
        }

        // OK, we've skipped the "\\?\", a UNC server\share or a drive letter with optional slash. We can now start
        // iterating the rest of the path.

        size_t rootLength = output.size();
        if (*input) {
            for (;;) {
                const TCHAR* componentEnd = input;
                while (*componentEnd && !Path::isSlash(*componentEnd)) {
                    ++componentEnd;
                }

                // "." and ".." cause FindFirstFile to yield the name of the directory (e.g., C:\Windows\. yields Windows).
                if (input[0] == '.') {
                    if (componentEnd == input + 1) {
                        input = SkipSlashes(componentEnd);
                        if (!*input) {
                            break;
                        }

                        continue;
                    }

                    if (input[1] == '.' && componentEnd == input + 2) {
                        if (output.size() > rootLength && Path::isSlash(output[output.size() - 1])) {
                            output.resize(output.size() - 1);
                        }

                        size_t previousSlash = output.find_last_of(TEXT("/\\"));
                        if (previousSlash == TCharString::npos || previousSlash <= rootLength) {
                            output.resize(rootLength);
                        } else {
                            output.resize(previousSlash + 1);
                        }

                        input = SkipSlashes(componentEnd);
                        if (!*input) {
                            break;
                        }

                        continue;
                    }
                }

                output.append(input, componentEnd);

                WIN32_FIND_DATA findData;
                HANDLE findHandle = FindFirstFile(output.c_str(), &findData);
                if (findHandle != INVALID_HANDLE_VALUE) {
                    output.resize(output.size() - (componentEnd - input));
                    output.append(findData.cFileName);
                    FindClose(findHandle);
                }

                input = SkipSlashes(componentEnd);
                if (!*input) {
                    break;
                }

                output += '\\';
            }
        }

        // Eliminate any trailing slash.
        if (output.size() > rootLength && Path::isSlash(output[output.size() - 1])) {
            output.resize(output.size() - 1);
        }

        return output;
    }
}

bool NormalisePath(std::string& normalised, const char* path, Log* log)
{
#if 0

            *normalised = TCharToChar(CanonicaliseWindowsPath(CharToTChar(path).c_str());
            return true;

#else

    TCharString fullPath;
    if (!GetWindowsFullPathName(CharToTChar(path).c_str(), &fullPath)) {
        log->logWindowsError(GetLastError(), path);
        return false;
    }

#ifdef USE_GETLONGPATHNAME

    TCharString longPath;
    if (!GetWindowsLongPathName(fullPath.c_str(), &longPath)) {
        //log->logWindowsError(GetLastError(), path);
        longPath = fullPath;
    }

    TCharString canon = CanonicaliseWindowsPath(longPath.c_str());

#else

    TCharString canon = CanonicaliseWindowsPath(fullPath.c_str());

#endif

    // "\\?\" is a platform detail so I want to hide it from cross platform code. Ideally any code that
    // passes a filename to a Win32 API function should prepend this if the path name is too long.
    // Note that "//?/" will have been converted to "\\?\" by this point.
    if (lstrncmp(canon.c_str(), TEXT("\\\\?\\"), 4) == 0) {
        normalised = TCharToChar(canon.c_str() + 4);
    } else {
        normalised = TCharToChar(canon);
    }

    return true;

#endif
}

bool IsSameFile(const char* a, const char* b, Log* log)
{
    // TODO: on Win2K, XP and later you can use GetFileInformationByHandle

    std::string norma;
    if (!NormalisePath(norma, a, log)) {
        norma = a;
    }

    std::string normb;
    if (!NormalisePath(normb, b, log)) {
        normb = b;
    }

    return norma == normb;
}
}
