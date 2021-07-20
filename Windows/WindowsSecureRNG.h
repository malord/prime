// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_WINDOWSSECURERNG_H
#define PRIME_WINDOWS_WINDOWSSECURERNG_H

#include "../Config.h"

#if PRIME_MSC_AND_NEWER(1300)

#define PRIME_HAVE_WINDOWSSECURERNG

#include "../Log.h"
#include "../RNGBase.h"

namespace Prime {

class PRIME_PUBLIC WindowsSecureRNG : public RNGBase<WindowsSecureRNG> {
public:
    WindowsSecureRNG();

    ~WindowsSecureRNG();

    /// Called for you if you don't call it.
    bool init(Log* log);

    void close();

    bool isInitialised() const { return true; }

    bool generateBytes(void* buffer, size_t bufferSize, Log* log);

    //
    // Fulfil the pattern required by RNGBase
    //

    typedef uint32_t Result;
    typedef uint32_t Seed;

    void seed(Seed) { }

    Result generate();
};

}

#endif

#endif
