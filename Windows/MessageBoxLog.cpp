// Copyright 2000-2021 Mark H. P. Lord

#include "MessageBoxLog.h"

namespace Prime {

MessageBoxLog::MessageBoxLog()
{
    _hwnd = NULL;
    _useMessageBoxLevel = LevelOutput;

    for (int i = LevelMin; i <= LevelMax; ++i) {
        setGlobalPrefixEnabledForLevel((Level)i, false);
        setLevelPrefix((Level)i, NULL);
    }
}

void MessageBoxLog::setHWnd(HWND hwnd)
{
    WindowsCriticalSection::ScopedLock lock(&_mutex);
    _hwnd = hwnd;
}

void MessageBoxLog::write(Level level, const char* string)
{
    WindowsCriticalSection::ScopedLock lock(&_mutex);

    const char* caption = NULL;
    UINT icon = 0;

    switch (level) {
    default:
        icon = MB_ICONINFORMATION;
        break;

    case LevelDeveloperWarning:
        caption = PRIME_LOCALISE("Developer Warning");
        icon = MB_ICONWARNING;
        break;

    case LevelInfo:
    case LevelOutput:
        caption = PRIME_LOCALISE("Information");
        icon = MB_ICONINFORMATION;
        break;

    case LevelNote:
        caption = PRIME_LOCALISE("Note");
        icon = MB_ICONINFORMATION;
        break;

    case LevelWarning:
        caption = PRIME_LOCALISE("Warning");
        icon = MB_ICONWARNING;
        break;

    case LevelError:
    case LevelFatalError:
        caption = PRIME_LOCALISE("Error");
        icon = MB_ICONSTOP;
        break;

    case LevelRuntimeError:
        caption = PRIME_LOCALISE("Runtime Error");
        icon = MB_ICONSTOP;
        break;
    }

    OutputDebugString(CharToTChar(string).c_str());

    if (level >= _useMessageBoxLevel) {
        char fullCaption[300] = "";
        if (getGlobalPrefix()) {
            StringCopy(fullCaption, sizeof(fullCaption), getGlobalPrefix());
            StringAppend(fullCaption, sizeof(fullCaption), " - ");
        }
        if (caption) {
            StringAppend(fullCaption, sizeof(fullCaption), caption);
        }

        MessageBox(_hwnd, CharToTChar(string).c_str(), CharToTChar(fullCaption).c_str(), MB_OK | icon);
    }
}
}
