// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSTERMINATIONHANDLER_H
#define PRIME_WINDOWS_WINDOWSTERMINATIONHANDLER_H

#include "WindowsConfig.h"

namespace Prime {

class PRIME_PUBLIC WindowsTerminationHandler {
public:
    WindowsTerminationHandler();

    ~WindowsTerminationHandler();

    // TODO: when not under a deadline, change this to return a bool to invoke the default behaviour?
    typedef void (*Callback)();

    static void ignoringCallback();

    void setQuitCallbacks(Callback callback)
    {
        setInterruptCallback(callback);
        setTerminateCallback(callback);
    }

    void setInterruptCallback(Callback callback);

    void setHangUpCallback(Callback callback);

    void setTerminateCallback(Callback callback);

    void setPipeCallback(Callback callback);

private:
    struct CtrlType {
        Callback callback;
        bool set;
    };

    CtrlType* findCtrlType(DWORD ctrlType);

    void set(DWORD ctrlType, Callback callback);
    static BOOL WINAPI handlerRoutine(DWORD ctrlType);
    void restore(DWORD ctrlType);

    CtrlType _ctrlC;
    CtrlType _close;

    static WindowsTerminationHandler* _singleton;
};
}

#endif
