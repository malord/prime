// Copyright 2000-2021 Mark H. P. Lord

#include "CommandLineParser.h"
#include "Log.h"
#include "NumberParsing.h"
#include "StringUtils.h"
#include <stdlib.h>
#include <string.h>

namespace {

inline bool IsSwitch(char ch)
{
#ifdef PRIME_OS_WINDOWS
    // There's an environment variable on Windows for setting the character used for switches, but it's
    // not widely used so I'm choosing to ignore it and just support both switch characters.
    return ch == '-' || ch == '/';
#else
    return ch == '-';
#endif
}
}

namespace Prime {

CommandLineParser::~CommandLineParser()
{
}

void CommandLineParser::construct()
{
    _state.argv = NULL;
    _allowImplicitLongOptions = false;
    _responseFileChar = INT_MIN;
    _responseFileLoader = NULL;
}

bool CommandLineParser::init(char** argv)
{
    reset();
    _state.argv = argv + 1;

    return true;
}

void CommandLineParser::reset()
{
    _state.argv = NULL;
    _state.opt = NULL;
    _state.filename = NULL;
    _state.noMoreOptions = false;
    _state.isLongOption = false;
}

bool CommandLineParser::next()
{
    const char* arg;

    if (_state.opt) {
        // Long options can never be followed by more options. This should have been caught by the relevant call
        // to getOption(), but it suggests getValue() has been used but the parameter was then never read by
        // fetchParameter(). I'm making this an assertion failure, but feel free to comment if I missed something.
        PRIME_ASSERT(!_state.isLongOption || !*_state.opt);

        // ... if you do comment out the assert, the extra text after the long option will be ignored.
        if (*_state.opt && !_state.isLongOption) {
            return true;
        }

        ++_state.argv;
        _state.opt = NULL;
    }

    for (;;) {
        arg = *_state.argv;
        if (!arg) {
            return false;
        }

        if (!_state.noMoreOptions && *arg == _responseFileChar) {
            ++_state.argv;
            ++arg;
            if (!*arg) {
                arg = *_state.argv;
                if (!arg) {
                    return false;
                }
                ++_state.argv;
            }
            _responseFileLoader->loadResponseFile(arg, &_state.argv, Log::getGlobal());

        } else {
            break;
        }
    }

    // '-' by itself is a filename.
    if (_state.noMoreOptions || !IsSwitch(arg[0]) || !arg[1]) {
        _state.filename = arg;
        ++_state.argv;
        return true;
    }

    // skip the '-'
    ++arg;

    if (IsSwitch(*arg)) {
        ++arg;

        if (!*arg) {
            // "--" by itself marks the end of the options - everything afterwards is treated as a filename
            ++_state.argv;
            _state.noMoreOptions = true;
            return next();
        }

        _state.isLongOption = true;
    } else {
        _state.isLongOption = false;
    }

    _state.opt = arg;
    return true;
}

bool CommandLineParser::equalLongOptionName(const char* have, const char* want, const char*& ptr,
    bool hasParam, bool hasFlag)
{
    for (;;) {
        if (*want == '-' || *want == '_') {
            // Allow "ignore-whitespace" to match "ignorewhitespace" and "ignore_whitespace".
            if (*have == *want) {
                ++have;
            }

            ++want;
            continue;
        }

        if (!*want) {
            bool isEnd = !*have;
            bool isParam = hasParam && *have == '=';
            bool isFlag = hasFlag && (*have == '-' || *have == '+');
            if (isEnd || isParam || isFlag) {
                ptr = have;
                return true;
            }

            return false;
        }

        if (!*have) {
            return false;
        }

        if (ASCIIToLower(*have) != ASCIIToLower(*want)) {
            return false;
        }

        ++have;
        ++want;
    }
}

bool CommandLineParser::readCommand(const char* words)
{
    if (!isFilename()) {
        return readOption(words);
    }

    if (const char* vbar = strchr(words, '|')) {
        for (;;) {
            char word[64];
            PRIME_ASSERTMSG((size_t)(vbar - words) < PRIME_COUNTOF(word), "You need to increase the size of word");

            StringCopy(word, sizeof(word), words, vbar - words);

            if (word[0]) {
                // Ignore empty words
                if (readCommand(word)) {
                    return true;
                }
            }

            if (!*vbar) {
                return false;
            }

            words = vbar + 1;

            vbar = strchr(words, '|');
            if (!vbar) {
                return readCommand(words);
            }
        }
    }

    return ASCIIEqualIgnoringCase(words, getFilename());
}

bool CommandLineParser::readOptionOrValueOrFlag(const char* option, bool* flag, bool hasParam)
{
    PRIME_ASSERT(!flag || !hasParam); // never tested that code path, it seems daft

    if (!_state.opt) {
        return false;
    }

    if (const char* vbar = strchr(option, '|')) {
        for (;;) {
            char alternate[64];
            PRIME_ASSERTMSG((size_t)(vbar - option) < PRIME_COUNTOF(alternate), "You need to increase the size of alternate");

            StringCopy(alternate, sizeof(alternate), option, vbar - option);

            if (alternate[0]) {
                // Ignore empty names
                if (readOptionOrValueOrFlag(alternate, flag, hasParam)) {
                    return true;
                }
            }

            if (!*vbar) {
                return false;
            }

            option = vbar + 1;

            vbar = strchr(option, '|');
            if (!vbar) {
                return readOptionOrValueOrFlag(option, flag, hasParam);
            }
        }
    }

    const char* optWas = _state.opt;
    size_t optionLength = strlen(option);

    // If we're testing for a flag, also test for "no-" prefix, e.g., --colour- equates to --no-colour.
    // Also now added a "disable-" prefix, e.g., --disable-colour.
    if (flag && optionLength > 1) {
        if (strncmp(option, "no-", 3) != 0 && strncmp(option, "disable-", 8) != 0) {
            char alternate[64];

            StringCopy(alternate, sizeof(alternate), "no-");
            StringAppend(alternate, sizeof(alternate), option);
            if (readOptionOrValueOrFlag(alternate, flag, hasParam)) {
                *flag = !*flag;
                return true;
            }

            StringCopy(alternate, sizeof(alternate), "disable-");
            StringAppend(alternate, sizeof(alternate), option);
            if (readOptionOrValueOrFlag(alternate, flag, hasParam)) {
                *flag = !*flag;
                return true;
            }
        }
    }

    if (optionLength == 1) {
        if (_state.isLongOption || *_state.opt != *option) {
            return false;
        }

        ++_state.opt;

    } else {
        if (!_state.isLongOption && !_allowImplicitLongOptions) {
            return false;
        }

        if (!equalLongOptionName(_state.opt, option, _state.opt, hasParam, flag != NULL)) {
            return false;
        }

        _state.isLongOption = true; // If _allowImplicitLongOptions is true, then this just became a long option.
    }

    const char* optEnd = _state.opt;

    if (flag) {
        if (*_state.opt == '-' || *_state.opt == '+') {
            *flag = (*_state.opt == '+');
            ++_state.opt;

            // need to check that nothing follows the + or - for a long option
            // equalLongOptionName does this for non-flags.
            if (_state.isLongOption && *_state.opt) {
                _state.opt = optWas;
                return false;
            }
        } else {
            *flag = true;
        }
    }

    StringCopy(_state.currentOption, sizeof(_state.currentOption), "-");
    if (_state.isLongOption) {
        StringAppend(_state.currentOption, sizeof(_state.currentOption), "-");
    }
    StringAppend(_state.currentOption, sizeof(_state.currentOption), optWas, optEnd - optWas);

    return true;
}

bool CommandLineParser::fetchBool()
{
    const char* optWas = _state.opt;
    char** argvWas = _state.argv;

    const char* value = fetchArgument(true);

    if (!value) {
        return true;
    }

    static const char* values[] = {
        "no",
        "yes",
        "false",
        "true",
        "off",
        "on",
        "0",
        "1",
        "-",
        "+",
    };

    for (int i = 0; i != (int)PRIME_COUNTOF(values); ++i) {
        if (ASCIIEqualIgnoringCase(values[i], value)) {
            return (i & 1) != 0;
        }
    }

    // Unknown value - put it back and assume a value hasn't been given.
    _state.opt = optWas;
    _state.argv = argvWas;
    return true;
}

const char* CommandLineParser::fetchArgument(bool optional)
{
    if (_state.opt) {
        const char* arg = _state.opt;

        ++_state.argv;
        _state.opt = NULL;

        if (*arg == '=' || *arg == ' ') {
            return arg + 1;
        }

        if (*arg) {
            return arg;
        }
    }

    if (!*_state.argv) {
        return NULL;
    }

    if (optional && IsSwitch(**_state.argv)) {
        return NULL;
    }

    return *_state.argv++;
}

const char* CommandLineParser::fetchString()
{
    const char* arg = fetchArgument(false);

    if (!arg) {
        exit(ExitReasonMissingArgument);
    }

    return arg;
}

intmax_t CommandLineParser::fetchIntmax()
{
    const char* arg = fetchString();

    intmax_t value = 0;
    if (!StringToInt(arg, value)) {
        exit(ExitReasonMissingArgument);
    }

    return value;
}

int CommandLineParser::fetchInt()
{
    intmax_t value = fetchIntmax();

    int truncated = (int)value;

    if (truncated != value) {
        exit(ExitReasonInvalidArgument);
    }

    return truncated;
}

intmax_t CommandLineParser::fetchOptionalIntmax(intmax_t defaultValue)
{
    State state(_state);

    if (const char* arg = fetchArgument(true)) {
        intmax_t value;
        if (StringToInt(arg, value)) {
            return value;
        }
    }

    _state = state;
    return defaultValue;
}

int CommandLineParser::fetchOptionalInt(int defaultValue)
{
    intmax_t value = fetchOptionalIntmax(defaultValue);

    int truncated = (int)value;

    if (truncated != value) {
        exit(ExitReasonInvalidArgument);
    }

    return truncated;
}

void CommandLineParser::skipLongOption()
{
    PRIME_ASSERT(_state.opt);
    _state.opt = "";
}

void CommandLineParser::skipShortOption()
{
    PRIME_ASSERT(_state.opt && *_state.opt);
    ++_state.opt;
}

double CommandLineParser::fetchDouble()
{
    const char* arg = fetchString();

    char* end;
    double value = strtod(arg, &end);
    if (end == arg || *end) {
        exit(ExitReasonMissingArgument);
    }

    return value;
}

float CommandLineParser::fetchFloat()
{
    double value = fetchDouble();
    if (value > FLT_MAX) {
        return FLT_MAX;
    }
    if (value < -FLT_MAX) {
        return -FLT_MAX;
    }

    return static_cast<float>(value);
}

bool CommandLineParser::readColourFlag(bool* flag)
{
    return readFlag("colour|colours|color|colors|G", flag);
}

void CommandLineParser::exit(ExitReason reason)
{
    switch (reason) {
    case ExitReasonMissingArgument:
        Log::getGlobal()->exitError("Missing argument to %s.", _state.currentOption);
        break;

    case ExitReasonInvalidArgument:
        Log::getGlobal()->exitError("Invalid argument to %s.", _state.currentOption);
        break;

    case ExitReasonUnknownOption:
        Log::getGlobal()->exitError("Unknown option: %s%s.", _state.isLongOption ? "--" : "-", getOption());
        break;

    case ExitReasonUnexpectedArgument:
        Log::getGlobal()->exitError("Unexpected argument: %s.", getFilename());
        break;

    case ExitReasonUnknownOptionOrUnexpectedArgument:
        if (isOption()) {
            exit(ExitReasonUnknownOption);
        } else {
            exit(ExitReasonUnexpectedArgument);
        }
        break;
    }

    Log::getGlobal()->exitError("Exiting for unknown reason.");
}
}
