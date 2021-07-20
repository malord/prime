// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WNDLIBLOG_H
#define PRIME_WINDOWS_WNDLIBLOG_H

#include "WindowsConfig.h"
#include "WndLib/LogWnd.h"

namespace Prime {

/// A Log implementation that uses a WndLib LogWnd.
class WndLibLog : public TextLog {
public:
    typedef WndLib::LogWnd LogWnd;

    WndLibLog()
    {
        for (int i = LevelMin; i <= LevelMax; ++i) {
            setGlobalPrefixEnabledForLevel((Level)i, false);
        }
    }

    ~WndLibLog()
    {
        if (_logWnd.GetHWnd()) {
            waitForClose();
        }
    }

    void create(const char* title, DWORD flags = 0, HWND parent = NULL)
    {
        _logWnd.Create(CharToTChar(title).c_str(), flags, parent);
    }

    void waitForClose()
    {
        _logWnd.SetForegroundWindow();
        _logWnd.WaitForUserToClose();
    }

protected:
    virtual void write(Level level, const char* string) PRIME_OVERRIDE
    {
        struct LevelSettings {
            COLORREF prefixColour;
            COLORREF textColour;
            LogWnd::ShowCommand showCommand;
        } levels[] = {
            { RGB(96, 128, 128), RGB(96, 164, 164), LogWnd::SHOWCOMMAND_NO_CHANGE }, // Trace
            { RGB(128, 128, 128), RGB(128, 128, 128), LogWnd::SHOWCOMMAND_NO_CHANGE }, // Verbose
            { RGB(0, 0, 0), RGB(0, 0, 0), LogWnd::SHOWCOMMAND_SHOW_IN_BACKGROUND }, // Output
            { RGB(0, 0, 0), RGB(0, 0, 0), LogWnd::SHOWCOMMAND_SHOW_IN_BACKGROUND }, // Information
            { RGB(0, 192, 0), RGB(0, 0, 0), LogWnd::SHOWCOMMAND_SHOW_IN_FOREGROUND }, // Note
            { RGB(255, 128, 0), RGB(0, 0, 0), LogWnd::SHOWCOMMAND_SHOW_IN_FOREGROUND }, // Warning
            { RGB(255, 0, 255), RGB(0, 0, 0), LogWnd::SHOWCOMMAND_SHOW_IN_BACKGROUND }, // Developer warning
            { RGB(192, 0, 0), RGB(0, 0, 0), LogWnd::SHOWCOMMAND_SHOW_IN_FOREGROUND }, // Error
            { RGB(255, 0, 64), RGB(0, 0, 0), LogWnd::SHOWCOMMAND_ALERT }, // Runtime Error
            { RGB(255, 0, 0), RGB(0, 0, 0), LogWnd::SHOWCOMMAND_ALERT }, // Fatal Error
        };

        const LevelSettings& settings = levels[level - Log::LevelTrace];

        if (const char* prefix = getLevelPrefix(level)) {
            if (*prefix) {
                char buffer[128];
                StringCopy(buffer, sizeof(buffer), prefix);
                StringAppend(buffer, sizeof(buffer), ": ");
                _logWnd.Log(CharToTChar(buffer).c_str(), settings.prefixColour, settings.showCommand);
            }
        }

        _logWnd.Log(CharToTChar(string).c_str(), settings.textColour, settings.showCommand);

        if (level == LevelFatalError) {
            waitForClose();
        }
    }

    virtual bool appendLevelPrefix(char*, size_t, Level)
    {
        return true;
    }

    LogWnd _logWnd;
};
}

#endif
