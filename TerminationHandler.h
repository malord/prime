// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TERMINATIONHANDLER_H
#define PRIME_TERMINATIONHANDLER_H

#include "Config.h"

#if defined(PRIME_OS_UNIX) && !defined(PRIME_OS_BB10)

#include "Unix/UnixTerminationHandler.h"

namespace Prime {
/// Typedef to the platform TerminationHandler, which handles process signals designed to terminate (e.g., SIGINT).
typedef UnixTerminationHandler TerminationHandler;
}

#elif defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsTerminationHandler.h"

namespace Prime {
/// Typedef to the platform TerminationHandler, which handles process signals designed to terminate (e.g., SIGINT).
typedef WindowsTerminationHandler TerminationHandler;
}

#else

namespace Prime {

class NullTerminationHandler {
public:
    typedef void (*Callback)();

    static void ignoringCallback() { }

    void init(Callback) { }

    void setInterruptCallback(Callback) { }

    void setHangUpCallback(Callback) { }

    void setTerminateCallback(Callback) { }

    void setPipeCallback(Callback) { }
};

typedef NullTerminationHandler TerminationHandler;
}

#endif

#endif
