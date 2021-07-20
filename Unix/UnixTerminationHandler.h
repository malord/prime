// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXTERMINATIONHANDLER_H
#define PRIME_UNIX_UNIXTERMINATIONHANDLER_H

#include "../Config.h"
#include <signal.h>
#include <unistd.h>

namespace Prime {

/// Takes care of setting up signal handlers for SIGINT (Ctrl+C), SIGHUP, SIGTERM and SIGPIPE.
class PRIME_PUBLIC UnixTerminationHandler {
public:
    UnixTerminationHandler();

    ~UnixTerminationHandler();

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
    struct Signal {
        struct sigaction oldAction;
        Callback callback;
        bool set;
    };

    Signal* findSignal(int signum);

    void set(int signum, Callback callback);
    static void callbackThunk(int signum);
    void restore(int signum);

    // If we start needing to handle a lot of signals, it'll be safer to just have an array Signal _signals[32]...
    Signal _interrupt;
    Signal _hangUp;
    Signal _terminate;
    Signal _pipe;

    static UnixTerminationHandler* _singleton;
};
}

#endif
