// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ANDROID_ANDROIDLOG_H
#define PRIME_ANDROID_ANDROIDLOG_H

#include "../Log.h"
#include <string>

namespace Prime {

class PRIME_PUBLIC AndroidLog : public Log {
public:
    AndroidLog();

    explicit AndroidLog(std::string tag);

    ~AndroidLog();

    //
    // Android log tag
    //

    void setTag(std::string tag) { _tag.swap(tag); }

    //
    // Level filtering
    //

    /// The default minimum level is LevelMin.
    void setLevel(Level level) { _level = level; }

    /// You should usually call isLevelEnabled() rather than test the result of this function, since it deals
    /// with developer mode.
    Level getLevel() const { return _level; }

    /// Returns true if the default global log would log this log level.
    bool isLevelEnabled(Level level) { return level >= _level; }

    /// Sets the minimum log level to LevelVerbose, or to LevelTrace if it's already at LevelVerbose or lower.
    /// Using this allows a command line tool to switch to trace mode by specifying verbose mode (usually, -v)
    /// twice (e.g., -v -v or -vv). -vvv enables developer mode.
    void increaseVerbosity();

    //
    // Log implementation
    //

    virtual bool logVA(Level, const char* format, va_list argptr) override;

private:
    Level _level;
    std::string _tag;
};
}

#endif
