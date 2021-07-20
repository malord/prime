// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_EMULATED_EMULATEDWILDCARDEXPANSION_H
#define PRIME_EMULATED_EMULATEDWILDCARDEXPANSION_H

#include "../DirectoryReader.h"

#ifdef PRIME_HAVE_DIRECTORYREADER

#include "../WildcardExpansionBase.h"

#define PRIME_HAVE_EMULATEDWILDCARDEXPANSION

namespace Prime {

/// If the platform provides a DirectoryReader but does not provide a WildcardExpansion then
/// EmulatedWildcardExpansion provides a simple WildcardExpansion (using basic wildcard matching).
class PRIME_PUBLIC EmulatedWildcardExpansion : public WildcardExpansionBase {
public:
    EmulatedWildcardExpansion();

    /// Immediately invokes find().
    explicit EmulatedWildcardExpansion(const char* pattern, const Options& options, Log* log);

    ~EmulatedWildcardExpansion();

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
    DirectoryReader _dir;
    bool _dirOpen;

    std::string _pattern;
    const char* _wildcard; // Points in to _pattern

    std::string _path;

    std::string _joined[2];
    int _joinedToUse;

    const char* _next;

    bool _foundAnyMatches;

    PRIME_UNCOPYABLE(EmulatedWildcardExpansion);
};

}

#endif

#endif
