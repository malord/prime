// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSWILDCARDEXPANSION_H
#define PRIME_WINDOWS_WINDOWSWILDCARDEXPANSION_H

#include "../Log.h"
#include "../WildcardExpansionBase.h"
#include "WindowsConfig.h"
#include "WindowsDirectoryReader.h"

namespace Prime {

/// A basic UNIX glob() workalike that expands a wildcard in to a list of matching file names. Similar to
/// EmulatedWildcardExpansion, but takes advantage of Windows' built-in wildcard matching and case insensitivity
/// (via FindFirstFile).
class PRIME_PUBLIC WindowsWildcardExpansion : public WildcardExpansionBase {
public:
    WindowsWildcardExpansion();

    /// Immediately invoke find().
    explicit WindowsWildcardExpansion(const char* pattern, const Options& options, Log* log);

    ~WindowsWildcardExpansion();

    /// Begins finding file names which match the specified pattern.
    bool find(const char* pattern, const Options& options, Log* log);

    /// Returns the next match.
    const char* read(Log* log);

    void close();

private:
    bool findNextMatch(Log* log);

    void construct();

    bool _begun;
    Options _options;
    WindowsDirectoryReader _dir;
    bool _dirOpen;

    std::string _pattern;
    const char* _wildcard; // Points in to _pattern

    std::string _path;

    std::string _joined[2];
    int _joinedToUse;

    const char* _next;

    bool _foundAnyMatches;

    PRIME_UNCOPYABLE(WindowsWildcardExpansion);
};

}

#endif
