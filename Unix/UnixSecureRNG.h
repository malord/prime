// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXSECURERNG_H
#define PRIME_UNIX_UNIXSECURERNG_H

#include "../Log.h"
#include "../RNGBase.h"

namespace Prime {

class PRIME_PUBLIC UnixSecureRNG : public RNGBase<UnixSecureRNG> {
public:
    UnixSecureRNG();

    ~UnixSecureRNG();

    /// Called for you if you don't call it.
    bool init(Log* log);

    void close();

    bool isInitialised() const { return _fd >= 0; }

    bool generateBytes(void* buffer, size_t bufferSize, Log* log);

    //
    // Fulfil the pattern required by RNGBase
    //

    typedef uint32_t Result;
    typedef uint32_t Seed;

    void seed(Seed) { }

    Result generate();

private:
    int _fd;
};

}

#endif
