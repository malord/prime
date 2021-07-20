// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_COMMANDLINERECODER_H
#define PRIME_COMMANDLINERECODER_H

#include "Config.h"

namespace Prime {

/// Figure out argc and argv, retrieving them as UTF-8 in Windows Unicode builds.
class PRIME_PUBLIC CommandLineRecoder {
public:
    /// Processes the supplied command line. On Windows, the Unicode command line is read using GetCommandLine()
    /// and converted to UTF-8. On other platforms, *argc and *argv are just stored.
    CommandLineRecoder(int* argc, char*** argv)
    {
        _allocedArgv = NULL;
        fixCommandLineEncoding(argc, argv, _allocedArgv);
        _argc = *argc;
        _argv = *argv;
    }

    /// Processes the supplied command line. On Windows, the Unicode command line is read using GetCommandLine()
    /// and converted to UTF-8 and the arguments are ignored. On other platforms, argc and argv must be correct.
    /// You can then retrieve the parsed command line with getArgc() and getArgv().
    CommandLineRecoder(int argc, char** argv)
    {
        _argc = argc;
        _argv = argv;
        _allocedArgv = NULL;
        fixCommandLineEncoding(&_argc, &_argv, _allocedArgv);
    }

    CommandLineRecoder()
    {
        _argc = 1;
        _argv = noArgs;
    }

    ~CommandLineRecoder() { freeEncodedCommandLine(_allocedArgv); }

    int getArgc() const { return _argc; }

    char** getArgv() const { return _argv; }

private:
    static void fixCommandLineEncoding(int* argc, char*** argv, char**& allocedArgv);

    static void freeEncodedCommandLine(char**& allocedArgv);

    int _argc;
    char** _argv;
    char** _allocedArgv;

    static char* noArgs[];

    PRIME_UNCOPYABLE(CommandLineRecoder);
};

}

#endif
