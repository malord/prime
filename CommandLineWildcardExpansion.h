// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_COMMANDLINEWILDCARDEXPANSION_H
#define PRIME_COMMANDLINEWILDCARDEXPANSION_H

#include "Config.h"

#if defined(PRIME_OS_WINDOWS)

#include "Windows/WindowsWildcardExpansion.h"

namespace Prime {

/// Typedef to a WildcardExpansion to use for filenames passed on the command line. On UNIX this will be a
/// do-nothing WildcardExpansion since wildcards are expanded by the shell.
typedef WindowsWildcardExpansion CommandLineWildcardExpansion;
}

#elif 0

// For platforms which have DirectoryReader and want to do wildcard expansion.

#include "Emulated/EmulatedWildcardExpansion.h"

namespace Prime {

/// Typedef to a WildcardExpansion to use for filenames passed on the command line. On UNIX this will be a
/// do-nothing WildcardExpansion since wildcards are expanded by the shell.
typedef EmulatedWildcardExpansion CommandLineWildcardExpansion;
}

#else

//
// Platforms where the shell is responsible for command line wildcard expansion.
//

#include "WildcardExpansionBase.h"
#include <string>

namespace Prime {

class Log;

/// @private
class NullWildcardExpansion : public WildcardExpansionBase {
public:
    NullWildcardExpansion() { }

    NullWildcardExpansion(const char* pattern, const Options& options, Log* log)
    {
        find(pattern, options, log);
    }

    bool find(const char* pattern, const Options&, Log* log)
    {
        (void)log;
        _match = pattern;
        _alreadyRead = false;
        return true;
    }

    const char* read(Log*)
    {
        if (_alreadyRead) {
            return NULL;
        }

        _alreadyRead = true;
        return _match.c_str();
    }

    void close() { _alreadyRead = true; }

private:
    std::string _match;
    bool _alreadyRead;

    PRIME_UNCOPYABLE(NullWildcardExpansion);
};

/// Typedef to a WildcardExpansion to use for filenames passed on the command line. On UNIX this will be a
/// do-nothing WildcardExpansion since wildcards are expanded by the shell.
typedef NullWildcardExpansion CommandLineWildcardExpansion;
}

#endif

#endif
