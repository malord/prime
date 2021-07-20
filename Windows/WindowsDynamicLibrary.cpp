// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsDynamicLibrary.h"
#include <string>

namespace Prime {

WindowsDynamicLibrary::WindowsDynamicLibrary()
{
    _module = 0;
}

WindowsDynamicLibrary::~WindowsDynamicLibrary()
{
    close();
}

DWORD WindowsDynamicLibrary::tryLoad(const char* name)
{
    SetLastError(NO_ERROR);
    _module = LoadLibrary(CharToTChar(name).c_str());

    if (_module) {
        return NO_ERROR;
    }

    DWORD error = GetLastError();
    return error != NO_ERROR ? error : ERROR_FILE_NOT_FOUND;
}

bool WindowsDynamicLibrary::load(const char* name)
{
    DWORD error;

    close();

    error = tryLoad(name);

    if (error == ERROR_FILE_NOT_FOUND) {
        // Try with a "lib" prefix (for UNIX compatibility)
        std::string prefixed("lib");
        prefixed.append(name);
        error = tryLoad(prefixed.c_str());
    }

    return error == NO_ERROR;
}

void WindowsDynamicLibrary::close()
{
    if (!_module) {
        return;
    }

    FreeLibrary(_module);
    _module = 0;
}

void* WindowsDynamicLibrary::findSymbol(const char* name)
{
    if (!_module) {
        return 0;
    }

    return GetProcAddress(_module, name);
}
}
