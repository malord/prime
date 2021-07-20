// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsTerminationHandler.h"

namespace Prime {

WindowsTerminationHandler* WindowsTerminationHandler::_singleton = NULL;

void WindowsTerminationHandler::ignoringCallback()
{
}

WindowsTerminationHandler::WindowsTerminationHandler()
{
    PRIME_ASSERT(!_singleton);
    _singleton = this;

    _ctrlC.set = false;
    _close.set = false;

    SetConsoleCtrlHandler(&WindowsTerminationHandler::handlerRoutine, TRUE);
}

WindowsTerminationHandler::~WindowsTerminationHandler()
{
    if (this == _singleton) {
        _singleton = NULL;
    }

    restore(CTRL_C_EVENT);
    restore(CTRL_CLOSE_EVENT);
}

WindowsTerminationHandler::CtrlType* WindowsTerminationHandler::findCtrlType(DWORD ctrlType)
{
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        return &_ctrlC;

    case CTRL_CLOSE_EVENT:
        return &_close;
    }

    return NULL;
}

void WindowsTerminationHandler::set(DWORD ctrlType, Callback callback)
{
    CtrlType* ctrl = findCtrlType(ctrlType);
    if (!ctrl) {
        return;
    }

    ctrl->callback = callback;
    ctrl->set = true;
}

BOOL WINAPI WindowsTerminationHandler::handlerRoutine(DWORD ctrlType)
{
    if (!_singleton) {
        return FALSE;
    }

    CtrlType* ctrl = _singleton->findCtrlType(ctrlType);
    if (!ctrl || !ctrl->set) {
        return FALSE;
    }

    (*ctrl->callback)();
    return TRUE;
}

void WindowsTerminationHandler::restore(DWORD ctrlType)
{
    if (CtrlType* ctrl = findCtrlType(ctrlType)) {
        ctrl->set = false;
    }
}

void WindowsTerminationHandler::setInterruptCallback(Callback callback)
{
    set(CTRL_C_EVENT, callback);
}

void WindowsTerminationHandler::setHangUpCallback(Callback callback)
{
    (void)callback;
}

void WindowsTerminationHandler::setTerminateCallback(Callback callback)
{
    set(CTRL_CLOSE_EVENT, callback);
}

void WindowsTerminationHandler::setPipeCallback(Callback callback)
{
    (void)callback;
}
}
