// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_COMMANDLINEPARSER_H
#define PRIME_COMMANDLINEPARSER_H

#include "Config.h"

namespace Prime {

class Log;

/// A command line reader that supports short option (-v) and long options (--verbose), combined short options
/// (e.g., -v -n -r can be shortened to -vnr) and -- to mark the end of the options. In option names containing
/// a '-', the '-' is optional (e.g., --nocolour will match "no-colour"). A flag is an option that may be followed
/// by a '-' to disable it, e.g., -G- or --colours-, or a + to enable it. Long name flags can also be specified
/// with a "no-" or "disable-" prefix to negate them, e.g., --no-colours has the same result as --colours-. It is
/// also possible to use long options by default, e.g., -trace instead of --trace, by using
/// setImplicitLongOptionsEnabled(), which defaults to false. Values are options which expect one or more
/// parameters, e.g., --dest ~/Desktop.
class PRIME_PUBLIC CommandLineParser {
public:
    CommandLineParser()
    {
        construct();
        reset();
    }

    explicit CommandLineParser(char** argv)
    {
        construct();
        init(argv);
    }

    virtual ~CommandLineParser();

    /// Set the arguments to be read. The last element argv must be null (i.e., like the argument array which
    /// is passed to main()). Note that returned strings are commonly pointers in to the strings in this array,
    /// so the array must remain valid until the command line has been read.
    bool init(char** argv);

    class PRIME_PUBLIC ResponseFileLoader {
    public:
        virtual ~ResponseFileLoader() { }

        /// Update ***argv to point to a new list of arguments to be parsed. The file name of the response file
        /// comes from path, which may itself come from another response file.
        virtual void loadResponseFile(const char* path, char*** argv, Log* log) = 0;
    };

    void setResponseFileLoader(char responseFileChar, ResponseFileLoader* responseFileLoader)
    {
        _responseFileChar = responseFileChar;
        _responseFileLoader = responseFileLoader;
    }

    void reset();

    /// If true, -trace will be considered to match --trace, rather than -t -r -a -c -e. Defaults to false. For
    /// this to work, the application must check for the long options before the short options.
    bool getImplicitLongOptionsEnabled() const { return _allowImplicitLongOptions; }

    void setImplicitLongOptionsEnabled(bool enabled) { _allowImplicitLongOptions = enabled; }

    /// Parse the next token from the argument list. Returns false if there are no more arguments to read.
    bool next();

    /// Returns true if a basic, not-an-option argument was read.
    bool isFilename() const { return !_state.opt; }

    /// If a file name was read, returns it.
    const char* getFilename() const { return _state.opt ? NULL : _state.filename; }

    /// Returns true if a "--" argument has been encountered, signifying that all remaining arguments are files.
    bool hasOptionTerminatorBeenRead() const { return _state.noMoreOptions; }

    /// Returns true if an option, value or flag was read.
    bool isOption() const { return _state.opt ? true : false; }

    /// Returns the option that hasn't been read (for use when reporting errors, don't compare this, use
    /// readOption(), readFlag() or readValue()).
    const char* getOption() const { return _state.opt; }

    /// Returns the last option that was successfully read (with readOption(), readFlag() or readValue()).
    const char* getCurrentOption() const { return _state.currentOption; }

    /// Return the option or filename that was parsed.
    const char* getOptionOrFilename() const { return _state.opt ? _state.opt : _state.filename; }

    /// Returns true if the next argument is one of the | separated words. For example, for an archive utility
    /// you might ask cl.readCommand("add|a"), which would match "add", "a", "--add" and "-a" (and "-add" if
    /// implicit long options are enabled).
    bool readCommand(const char* words);

    /// Returns true if the specified option was read. e.g., readOption("verbose|v")
    bool readOption(const char* option)
    {
        return readOptionOrValueOrFlag(option, NULL, false);
    }

    /// If the specified option was read, returns true and sets *flag to true or false depending on whether the
    /// option was followed by a + or -, respectively. So -f or -f+ would set *flag to true, -f- to false.
    /// If flag is NULL, the result is stored internally and can be read by calling getFlag().
    bool readFlag(const char* option, bool* flag = NULL)
    {
        return readOptionOrValueOrFlag(option, flag ? flag : &_state.flag, false);
    }

    /// Returns the flag read by readFlag() (or readColourFlag()) if they were called with a NULL flag pointer.
    bool getFlag() const { return _state.flag; }

    /// Returns true if the specified option, which should have a value, was read. After calling this you should
    /// call one of the fetch*() methods (fetchString(), fetchInt() etc.) to fetch the option's value. A value
    /// differs from a plain option in that it may be followed by an '=' sign, e.g., `--path=/bin`, which could
    /// also be supplied as `--path /bin` and `--path= /bin`. An option can have multiple values, e.g.,
    /// `--offset 160 120`, and the fetch*() methods should be called for each.
    bool readValue(const char* option)
    {
        return readOptionOrValueOrFlag(option, NULL, true);
    }

    /// Fetch a string from the command line. Exits if there are no more arguments.
    const char* fetchString();

    /// Fetch an intmax_t from the command line. Exits if there are no more arguments or the argument is invalid.
    intmax_t fetchIntmax();

    /// Fetch an int from the command line. Exits if there are no more arguments or the argument is invalid.
    int fetchInt();

    /// Fetch an intmax_t from the command line. If the next argument isn't a valid number, returns the default
    /// value and leaves the next argument to be read.
    intmax_t fetchOptionalIntmax(intmax_t defaultValue);

    /// Fetch an int from the command line. If the next argument isn't a valid number, returns the default
    /// value and leaves the next argument to be read.
    int fetchOptionalInt(int defaultValue);

    /// Fetch a float from the command line. Exits if there are no more arguments or the argument is invalid.
    float fetchFloat();

    /// Fetch a double from the command line. Exits if there are no more arguments or the argument is invalid.
    double fetchDouble();

    /// Fetch the next argument and convert the result to a bool. If there's no argument, or the next argument
    /// begins with the switch character (- or /) then true is assumed, but if there is an argument then yes,
    /// true, on, 1 and + are all considered true and no, false, off, 0 and - are all considered false. So,
    /// -f 1, -f+, -f and even -f YES are all considered true. -f -x will be considered true, and -x will
    /// correctly be read next.
    bool fetchBool();

    /// Reads the standard colour/no colour flags (colour|color|colours|colors|G).
    bool readColourFlag(bool* flag = NULL);

    void skipLongOption();

    void skipShortOption();

    /// Skip an option's value. If unlessOption is true, if the next argument begins with a - then treat it as
    /// an option and don't skip it.
    void skipValue(bool unlessOption = false)
    {
        (void)fetchArgument(unlessOption);
    }

    // You can overload exit(ExitReason) to change how these are handled.

    void exitDueToMissingArgument() { exit(ExitReasonMissingArgument); }
    void exitDueToInvalidArgument() { exit(ExitReasonInvalidArgument); }
    void exitDueToUnknownOption() { exit(ExitReasonUnknownOption); }
    void exitDueToUnexpectedArgument() { exit(ExitReasonUnexpectedArgument); }
    void exitDueToUnknownOptionOrUnexpectedArgument() { exit(ExitReasonUnknownOptionOrUnexpectedArgument); }

protected:
    enum ExitReason {
        ExitReasonMissingArgument,
        ExitReasonInvalidArgument,
        ExitReasonUnknownOption,
        ExitReasonUnexpectedArgument,
        ExitReasonUnknownOptionOrUnexpectedArgument,
    };

    virtual void exit(ExitReason reason);

private:
    void construct();

    bool readOptionOrValueOrFlag(const char* option, bool* flag, bool hasParam);

    static bool equalLongOptionName(const char* have, const char* want, const char*& ptr, bool hasParam, bool hasFlag);

    const char* fetchArgument(bool optional);

    struct State {
        char** argv;
        const char* opt;
        const char* filename;

        bool noMoreOptions;
        bool isLongOption;

        bool flag;
        char currentOption[64];
    } _state;

    bool _allowImplicitLongOptions;

    int _responseFileChar;
    ResponseFileLoader* _responseFileLoader;

    PRIME_UNCOPYABLE(CommandLineParser);
};
}

#endif
