// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXWILDCARDEXPANSION_H
#define PRIME_UNIX_UNIXWILDCARDEXPANSION_H

#include "../Platform.h"

// No glob.h on Android
#ifndef PRIME_OS_ANDROID

#include "../Log.h"
#include "../WildcardExpansionBase.h"
#include <glob.h>

namespace Prime {

/// A wrapper around the UNIX glob() API.
class PRIME_PUBLIC UnixWildcardExpansion : public WildcardExpansionBase {
public:
    UnixWildcardExpansion();

    /// Immediately invoke find().
    explicit UnixWildcardExpansion(const char* pattern, const Options& options, Log* log);

    ~UnixWildcardExpansion();

    /// Begins finding file names which match the specified pattern.
    bool find(const char* pattern, const Options& options, Log* log);

    /// Returns the next match.
    const char* read(Log* log);

    void close();

private:
    void construct();

    bool _globbed;
    glob_t _globStruct;

    ptrdiff_t _next;

    PRIME_UNCOPYABLE(UnixWildcardExpansion);
};
}

#endif // PRIME_OS_ANDROID

#endif
