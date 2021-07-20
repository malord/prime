// Copyright 2000-2021 Mark H. P. Lord

#include "UnixTerminationHandler.h"

namespace Prime {

UnixTerminationHandler* UnixTerminationHandler::_singleton = NULL;

void UnixTerminationHandler::ignoringCallback()
{
}

UnixTerminationHandler::UnixTerminationHandler()
{
    PRIME_ASSERT(!_singleton);
    _singleton = this;

    _interrupt.set = false;
    _hangUp.set = false;
    _terminate.set = false;
    _pipe.set = false;
}

UnixTerminationHandler::~UnixTerminationHandler()
{
    restore(SIGINT);
    restore(SIGHUP);
    restore(SIGTERM);
    restore(SIGPIPE);

    if (this == _singleton) {
        _singleton = NULL;
    }
}

UnixTerminationHandler::Signal* UnixTerminationHandler::findSignal(int signum)
{
    switch (signum) {
    case SIGINT:
        return &_interrupt;

    case SIGHUP:
        return &_hangUp;

    case SIGTERM:
        return &_terminate;

    case SIGPIPE:
        return &_pipe;
    }

    return NULL;
}

void UnixTerminationHandler::set(int signum, Callback callback)
{
    Signal* sig = findSignal(signum);
    if (!sig) {
        return;
    }

    if (!sig->set) {
        sigaction(signum, NULL, &sig->oldAction);
    }

    sig->callback = callback;

    struct sigaction newAction;
    if (callback == &UnixTerminationHandler::ignoringCallback) {
        newAction.sa_handler = SIG_IGN;
    } else {
        newAction.sa_handler = &UnixTerminationHandler::callbackThunk;
    }
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = 0;

    if (sig->oldAction.sa_handler != SIG_IGN) {
        sigaction(signum, &newAction, NULL);
    }

    sig->set = true;
}

void UnixTerminationHandler::callbackThunk(int signum)
{
    if (const Signal* sig = _singleton->findSignal(signum)) {
        (*sig->callback)();
    }
}

void UnixTerminationHandler::restore(int signum)
{
    Signal* sig = findSignal(signum);
    if (sig && sig->set) {
        sigaction(signum, &sig->oldAction, NULL);
        sig->set = false;
    }
}

void UnixTerminationHandler::setInterruptCallback(Callback callback)
{
    set(SIGINT, callback);
}

void UnixTerminationHandler::setHangUpCallback(Callback callback)
{
    set(SIGHUP, callback);
}

void UnixTerminationHandler::setTerminateCallback(Callback callback)
{
    set(SIGTERM, callback);
}

void UnixTerminationHandler::setPipeCallback(Callback callback)
{
    set(SIGPIPE, callback);
}
}
