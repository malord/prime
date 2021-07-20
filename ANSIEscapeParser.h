// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ANSIESCAPEPARSER_H
#define PRIME_ANSIESCAPEPARSER_H

#include "Config.h"

namespace Prime {

/// Parse ANSI escape sequences from text. Currently only deals with colours.
class PRIME_PUBLIC ANSIEscapeParser {
public:
    int foreground;
    int background;
    bool bold;

    ANSIEscapeParser()
        : _state(StateNone)
        , _codesTop(0)
        , foreground(-1)
        , background(-1)
        , bold(false)
    {
    }

    /// Returns a pointer to the first character after any ANSI commands have been parsed.
    const char* process(const char* ptr, const char* endPtr)
    {
        for (; ptr != endPtr; ++ptr) {
            char ch = *ptr;
            switch (_state) {
            case StateNone:
                if (ch != 27) {
                    return ptr;
                }

                _state = StateBegin;
                break;

            case StateBegin:
                if (ch != '[') {
                    _state = StateNone;
                    return ptr;
                }

                _state = StateDigits;
                _codes[0] = 0;
                _codesTop = 0;
                break;

            case StateDigits:
                if (ch >= '0' && ch <= '9') {
                    _codes[_codesTop] = _codes[_codesTop] * 10 + (ch - '0');
                    break;
                }

                if (ch == ';') {
                    ++_codesTop;
                    if (_codesTop == (int)COUNTOF(_codes)) {
                        --_codesTop;
                    }
                    _codes[_codesTop] = 0;

                    break;
                }

                if (ch == 'm') {
                    for (int i = 0; i <= _codesTop; ++i) {
                        int escapeCode = _codes[i];

                        if (escapeCode >= 30 && escapeCode <= 37) {
                            foreground = escapeCode - 30;
                        } else if (escapeCode >= 40 && escapeCode <= 47) {
                            background = escapeCode - 40;
                        } else if (escapeCode >= 90 && escapeCode <= 97) {
                            foreground = escapeCode - 90 + 8;
                        } else if (escapeCode >= 100 && escapeCode <= 107) {
                            background = escapeCode - 100 + 8;
                        } else if (escapeCode == 1) {
                            bold = true;
                        } else if (escapeCode == 0) {
                            foreground = -1;
                            background = -1;
                            bold = false;
                        }
                    }
                }

                _state = StateNone;
                break;
            }
        }

        return ptr;
    }

    void begin() { _state = StateBegin; }

private:
    enum State {
        StateNone,
        StateBegin,
        StateDigits
    };

    State _state;
    int _codes[20];
    int _codesTop;
};

}

#endif
