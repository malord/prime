// Copyright 2000-2021 Mark H. P. Lord

#include "ANSILog.h"
#include "ANSIEscapes.h"
#include "StringUtils.h"

#define BOLD PRIME_ANSI_BOLD ";"

namespace Prime {

ANSILog::ANSILog()
    : _brightColours(true)
{
    std::fill(_isTTY, &_isTTY[COUNTOF(_isTTY)], -1);
}

ANSILog::~ANSILog()
{
}

bool ANSILog::appendLevelPrefix(char* buffer, size_t bufferSize, Level level)
{
    const char* levelPrefix = getLevelPrefix(level);
    if (!levelPrefix || !*levelPrefix) {
        return true;
    }

    if (!shouldUseColour(isColourSupportedForLevel(level))) {
        return ConsoleLog::appendLevelPrefix(buffer, bufferSize, level);
    }

    const char* colour = NULL;

    switch (level) {
    case LevelNone:
        colour = NULL;
        break;

    case LevelFatalError:
    case LevelRuntimeError:
        if (getBrightColoursEnabled()) {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_BRIGHT_RED PRIME_ANSI_SET_ATTRIBUTE;
        } else {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_RED PRIME_ANSI_SET_ATTRIBUTE;
        }
        break;

    case LevelError:
        if (doesTerminalHaveDarkBackground() && getBrightColoursEnabled()) {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_BRIGHT_RED PRIME_ANSI_SET_ATTRIBUTE;
        } else {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_RED PRIME_ANSI_SET_ATTRIBUTE;
        }
        break;

    case LevelWarning:
        if (doesTerminalHaveDarkBackground() && getBrightColoursEnabled()) {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_BRIGHT_YELLOW PRIME_ANSI_SET_ATTRIBUTE;
        } else {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_MAGENTA PRIME_ANSI_SET_ATTRIBUTE;
        }
        break;

    case LevelNote:
        if (doesTerminalHaveDarkBackground() && getBrightColoursEnabled()) {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_BRIGHT_GREEN PRIME_ANSI_SET_ATTRIBUTE;
        } else {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_GREEN PRIME_ANSI_SET_ATTRIBUTE;
        }
        break;

    case LevelInfo:
        colour = NULL;
        break;

    case LevelDeveloperWarning:
        if (getBrightColoursEnabled()) {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_BRIGHT_MAGENTA PRIME_ANSI_SET_ATTRIBUTE;
        } else {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_MAGENTA PRIME_ANSI_SET_ATTRIBUTE;
        }
        break;

    case LevelVerbose:
        if (getBrightColoursEnabled()) {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_BRIGHT_BLACK PRIME_ANSI_SET_ATTRIBUTE;
        } else {
            colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_CYAN PRIME_ANSI_SET_ATTRIBUTE;
        }
        break;

    case LevelTrace:
        colour = PRIME_ANSI_CSI BOLD PRIME_ANSI_FG_CYAN PRIME_ANSI_SET_ATTRIBUTE;
        break;

    case LevelOutput:
        colour = NULL;
        break;
    }

    bool fit = true;

    if (colour) {
        fit = fit && StringAppend(buffer, bufferSize, colour);
    }

    fit = fit && StringAppend(buffer, bufferSize, levelPrefix);

    if (colour) {
        fit = fit && StringAppend(buffer, bufferSize, PRIME_ANSI_SEQUENCE_RESET_ATTRIBUTES);
    }

    fit = fit && StringAppend(buffer, bufferSize, ": ");

    return fit;
}

bool ANSILog::appendLog(char* buffer, size_t bufferSize, Level level, const char* format, va_list argptr)
{
    const char* colour = NULL;

    switch (level) {
    case LevelTrace:
        colour = PRIME_ANSI_CSI PRIME_ANSI_FG_CYAN PRIME_ANSI_SET_ATTRIBUTE;
        break;

    case LevelVerbose:
        if (getBrightColoursEnabled()) {
            colour = PRIME_ANSI_CSI PRIME_ANSI_FG_BRIGHT_BLACK PRIME_ANSI_SET_ATTRIBUTE;
        }
        break;

    default:
        break;
    }

    if (!colour || !shouldUseColour(isColourSupportedForLevel(level))) {
        return ConsoleLog::appendLog(buffer, bufferSize, level, format, argptr);
    }

    bool fit = true;

    fit = fit && StringAppend(buffer, bufferSize, colour);

    fit = fit && ConsoleLog::appendLog(buffer, bufferSize, level, format, argptr);

    if (StringEndsWith(buffer, '\n')) {
        buffer[strlen(buffer) - 1] = 0; // ConsoleLog will add this back, we need the newline after the ANSI reset
    }

    fit = fit && StringAppend(buffer, bufferSize, PRIME_ANSI_SEQUENCE_RESET_ATTRIBUTES);

    return fit;
}

bool ANSILog::getCachedIsATTYForLevel(Level level, bool& result) const
{
    if (isValidLevel(level)) {
        const int cache = _isTTY[level - LevelMin];
        if (cache < 0) {
            return false;
        }

        result = cache != 0;
        return true;
    }

    return false;
}

void ANSILog::setCachedIsATTYForLevel(Level level, bool value) const
{
    if (isValidLevel(level)) {
        int& cache = _isTTY[level - LevelMin];
        cache = value ? 1 : 0;
    }
}
}
