// Copyright 2000-2021 Mark H. P. Lord

#include "UnixDynamicLibrary.h"
#include "../StringUtils.h"
#include <dlfcn.h>
#include <stdio.h>
#include <sys/param.h>
#include <unistd.h>

namespace Prime {

UnixDynamicLibrary::UnixDynamicLibrary()
{
    _dl = 0;
}

UnixDynamicLibrary::~UnixDynamicLibrary()
{
    close();
}

bool UnixDynamicLibrary::tryLoad(const char* name)
{
    _dl = dlopen(name, 0);

    return (_dl != 0);
}

bool UnixDynamicLibrary::load(const char* name)
{
    close();

    bool success = tryLoad(name);

    if (!success) {
        // Try with a "lib" prefix.
        char lib[256];
        StringCopy(lib, "lib");
        if (StringAppend(lib, name)) {
            success = tryLoad(lib);
        }
    }

    return success;
}

void UnixDynamicLibrary::close()
{
    if (!_dl) {
        return;
    }

    dlclose(_dl);
    _dl = 0;
}

UnixDynamicLibrary::FunctionPointer UnixDynamicLibrary::findSymbol(const char* name)
{
    if (!_dl) {
        return 0;
    }

    // Avoid compiler warning about casting to a function pointer.
    union {
        void* ptr;
        FunctionPointer fp;
    } p;

    p.ptr = dlsym(_dl, name);
    if (!p.ptr) {
        return 0;
    }

    return p.fp;
}
}
