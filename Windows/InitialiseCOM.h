// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_INITIALISECOM_H
#define PRIME_WINDOWS_INITIALISECOM_H

#include "WindowsConfig.h"

#ifdef PRIME_OS_WINDOWS
#include <unknwn.h>
#endif

namespace Prime {

#ifdef PRIME_OS_WINDOWS

/// Initialises COM, de-initialising it when destructed.
class InitialiseCOM {
    HRESULT _result;

public:
    InitialiseCOM()
    {
#if PRIME_MSC_AND_OLDER(1300)
        // CoInitializeEx is available on Win98 and on Win95 with a DCOM patch, but the VC6 Windows SDK
        // doesn't declare it and the VC6 project I maintain has never been tested with
        // COINIT_MULTITHREADED.
        _result = CoInitialize(NULL);
#else
        _result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif
    }

    ~InitialiseCOM()
    {
        if (SUCCEEDED(_result)) {
            CoUninitialize();
        }
    }

    /// Returns true if initialisation of COM failed.
    bool operator!() const { return FAILED(_result); }
};

#else

/// Null version of InitialiseCOM for non-Windows platforms.
class InitialiseCOM {
public:
    /// The parameter stops some compilers commplaining about the construction of an object being a no-op.
    InitialiseCOM(bool = true) { }

    /// Always returns false (i.e., pretend initialise of COM succeeded).
    bool operator!() const { return false; }
};

#endif
}

#endif
