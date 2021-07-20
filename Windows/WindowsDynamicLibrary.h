// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSDYNAMICLIBRARY_H
#define PRIME_WINDOWS_WINDOWSDYNAMICLIBRARY_H

#include "WindowsConfig.h"

namespace Prime {

/// Load a DLL and function functions/symbols within it.
class PRIME_PUBLIC WindowsDynamicLibrary {
public:
    WindowsDynamicLibrary();

    ~WindowsDynamicLibrary();

    /// Returns true if a DLL has been loaded.
    bool isLoaded() const { return _module != 0; }

    /// Load the specified dynamic library. If we already have an open dynamic library, that one is closed first.
    /// Returns false on error.
    bool load(const char* name);

    /// Unload the dynamic library.
    void close();

    /// Access a symbol in the dynamic library. If the symbol is not found, returns a null pointer.
    void* findSymbol(const char* name);

private:
    DWORD tryLoad(const char* name);

    HMODULE _module;

    PRIME_UNCOPYABLE(WindowsDynamicLibrary);
};

}

#endif
