// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_MESSAGEBOXLOG_H
#define PRIME_WINDOWS_MESSAGEBOXLOG_H

#include "../TextLog.h"
#include "WindowsCriticalSection.h"

namespace Prime {

/// Displays output, info, notes, warnings and errors in a MessageBox. Trace and verbose messages go to
/// OutputDebugString, as do developer warnings unless in developer mode.
class MessageBoxLog : public Prime::TextLog {
public:
    MessageBoxLog();

    /// Set the window the message boxes should have as their parent. If NULL, the foreground window is used.
    void setHWnd(HWND hwnd);

    /// Set whether or not to use a MessageBox. If false, only writes to OutputDebugString. Defaults to true.
    void setMessageBoxLevel(Level level) { _useMessageBoxLevel = level; }

protected:
    virtual void write(Level level, const char* string) PRIME_OVERRIDE;

    HWND _hwnd;
    WindowsCriticalSection _mutex;
    Level _useMessageBoxLevel;
};

}

#endif
