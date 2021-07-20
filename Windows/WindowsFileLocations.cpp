// Copyright 2000-2021 Mark H. P. Lord

#include "../FileLocations.h"
#include "../Path.h"
#include "../ScopedPtr.h"
#include "WindowsConfig.h"
#ifndef PRIME_OS_XBOX
#include <shlwapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "shlwapi.lib")
#endif
#pragma warning(disable : 4091)
#include <shlobj.h>

#ifndef CSIDL_FLAG_CREATE
#define CSIDL_FLAG_CREATE 0
#endif
#endif

namespace Prime {

static std::string GetWindowsCommonPath(DWORD folderID)
{
    TCHAR path[MAX_PATH];
    SetLastError(0);

    /*if (FAILED(SHGetFolderPath(0, folderID, 0, SHGFP_TYPE_CURRENT, path))) {
            return std::string();
        }*/

    if (!SHGetSpecialFolderPath(NULL, path, folderID, TRUE)) {
        return std::string();
    }

    return TCharToChar(path);
}

std::string GetExecutableFilePath(const char*, Log*)
{
    const DWORD capacity = 32768u;
    ScopedArrayPtr<TCHAR> buf(new TCHAR[capacity]);

    DWORD result = GetModuleFileName(GetModuleHandle(NULL), buf.get(), capacity);
    if (!result || result >= capacity) {
        return "";
    }

    return TCharToChar(buf.get());
}

std::string GetToolsPath(const char* argv0, Log* log)
{
    return Path::stripLastComponent(GetExecutableFilePath(argv0, log));
}

std::string GetResourcesPath(const char* argv0, Log* log)
{
    return Path::stripLastComponent(GetExecutableFilePath(argv0, log));
}

std::string GetSavePath(const char* appID, Log*)
{
    std::string savePath = GetWindowsCommonPath(CSIDL_APPDATA | CSIDL_FLAG_CREATE);
    Path::joinInPlace(savePath, AppIDToRelativePath(appID));

    return savePath;
}

std::string GetPluginsPath(const char* appID, Log* log)
{
    return GetSavePath(appID, log);
}

std::string GetTemporaryPath(Log*)
{
    TCHAR stack[MAX_PATH];
    ScopedArrayPtr<TCHAR> heap;
    TCHAR* got;

    DWORD result = GetTempPath(COUNTOF(stack) - 1, stack);

    if (result >= COUNTOF(stack) - 1) {
        heap.reset(new TCHAR[result + 2]);
        result = GetTempPath(result + 1, heap.get());
        got = heap.get();

    } else {
        got = stack;
    }

    if (result == 0) {
        return "";
    }

    CreateDirectory(got, NULL);

    return TCharToChar(got);
}

std::string GetCachePath(const char* appID, Log* log)
{
    return Path::join(GetTemporaryPath(log), AppIDToRelativePath(appID));
}

std::string GetSystemPluginsPath(const char* appID, Log*)
{
#ifdef CSIDL_COMMON_APPDATA
    std::string systemSavePath = GetWindowsCommonPath(CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE);
#else
    std::string systemSavePath = GetWindowsCommonPath(CSIDL_APPDATA | CSIDL_FLAG_CREATE);
#endif
    Path::joinInPlace(systemSavePath, AppIDToRelativePath(appID));

    return systemSavePath;
}

std::string GetDesktopPath(Log*)
{
    std::string desktopPath = GetWindowsCommonPath(CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE);
    Path::stripTrailingSlashesInPlace(desktopPath);

    return desktopPath;
}
}
