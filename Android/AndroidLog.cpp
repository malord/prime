// Copyright 2000-2021 Mark H. P. Lord

#include "AndroidLog.h"
#include <android/log.h>

namespace Prime {

AndroidLog::AndroidLog()
    : _level(LevelMin)
    , _tag("Prime")
{
}

AndroidLog::AndroidLog(std::string tag)
    : _level(LevelMin)
{
    tag.swap(_tag);
}

AndroidLog::~AndroidLog()
{
}

void AndroidLog::increaseVerbosity()
{
    if (getLevel() <= LevelTrace) {
        SetDeveloperMode(true);
    } else if (getLevel() <= LevelVerbose) {
        setLevel(LevelTrace);
    } else {
        setLevel(LevelVerbose);
    }
}

bool AndroidLog::logVA(Level level, const char* format, va_list argptr)
{
    if (isLevelEnabled(level)) {
        android_LogPriority priority = ANDROID_LOG_DEBUG;
        switch (level) {
        case LevelNone:
        case LevelTrace:
            // On some phones, ANDROID_LOG_VERBOSE is *insane*, so don't use it
        case LevelVerbose:
            priority = ANDROID_LOG_DEBUG;
            break;
        case LevelOutput:
        case LevelInfo:
        case LevelNote:
            priority = ANDROID_LOG_INFO;
            break;
        case LevelWarning:
        case LevelDeveloperWarning:
            priority = ANDROID_LOG_WARN;
            break;
        case LevelError:
            priority = ANDROID_LOG_ERROR;
            break;
        case LevelRuntimeError:
        case LevelFatalError:
            priority = ANDROID_LOG_FATAL;
            break;
        }
        __android_log_vprint(ANDROID_LOG_INFO, _tag.c_str(), format, argptr);
    }

    return false;
}
}
