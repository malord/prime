// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXDYNAMICLIBRARY_H
#define PRIME_UNIX_UNIXDYNAMICLIBRARY_H

#include "../Config.h"

namespace Prime {

/// Represents a dynamic library that is opened with the UNIX function dlopen.
class PRIME_PUBLIC UnixDynamicLibrary {
public:
    /// Function pointer type returned by findSymbol().
    typedef void (*FunctionPointer)();

    UnixDynamicLibrary();

    ~UnixDynamicLibrary();

    /// Returns true if a dynamic library has been loaded.
    bool isLoaded() const { return _dl != 0; }

    /// Load the specified dynamic library. If we already have an open dynamic library, that one is closed first.
    /// Returns false on error.
    bool load(const char* name);

    /// Unload the dynamic library.
    void close();

    /// Access a symbol in the dynamic library. If the symbol is not found, returns a null pointer.
    FunctionPointer findSymbol(const char* name);

private:
    bool tryLoad(const char* name);

    void* _dl;

    PRIME_UNCOPYABLE(UnixDynamicLibrary);
};
}

#endif
