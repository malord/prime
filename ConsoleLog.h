// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CONSOLELOG_H
#define PRIME_CONSOLELOG_H

#include "TextLog.h"

namespace Prime {

/// Abstract base class for Log implementations which provide logging to a console/TTY. Implementations just
/// need to override write(). Some of the options (e.g., use stdout for log level, use OutputDebugString) are
/// platform specific, but are made available here to allow applications to set them without having to ifdef
/// which platform is being built.
class PRIME_PUBLIC ConsoleLog : public TextLog {
    PRIME_DECLARE_UID_CAST(TextLog, 0xcf8d4e98, 0x5f7c4906, 0x9f70b0cf, 0xe44443de)

public:
    ConsoleLog();

    //
    // Use stdout instead of stderr
    // By default, LevelOutput goes to stdout and everything else goes to stderr.
    //

    /// If false, stderr is used. By default, stderr is used for all levels except LevelOutput.
    void setUseStdoutForLevel(Level level, bool useStdout = true);

    bool getUseStdoutForLevel(Level level) const;

    void setUseStdoutForAllLevels();

    //
    // OutputDebugString
    //

    /// If enabled, all debug output goes to OutputDebugString rather than stdout/stderr (Windows only).
    void setOnlyUseOutputDebugString(bool enabled = true) { _useOutputDebugString = enabled; }

    bool getOnlyUseOutputDebugString() const { return _useOutputDebugString; }

    //
    // Colours
    //

    /// Force the use or non-use of colourised output. By default, colourised output is used where appropriate
    /// (e.g., when writing to a TTY, but not when writing to a file).
    void setColourEnabled(bool useColours);

    void setColourEnabledWherePossible() { _coloursEnabled = -1; }

    /// knownSupported should be true if output is to a colour-supporting TTY, false otherwise (e.g., it's to
    /// a file).
    bool shouldUseColour(bool knownSupported) const { return _coloursEnabled < 0 ? knownSupported : _coloursEnabled == 0 ? false : true; }

protected:
    /// Override TextLog to hide the global prefix if we have colours enabled.
    virtual bool shouldLevelHaveGlobalPrefix(Level level) const PRIME_OVERRIDE;

private:
    bool _levelUsesStdout[LevelMax - LevelMin + 1];

    bool _useOutputDebugString;

    int _coloursEnabled;

    PRIME_UNCOPYABLE(ConsoleLog);
};
}

#endif
