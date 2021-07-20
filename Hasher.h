// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_HASHER_H
#define PRIME_HASHER_H

#include "Config.h"
#include "RefCounting.h"
#include <vector>

namespace Prime {

/// Provides a polymorphic interface to objects capable of providing a hash for binary data (e.g., SHA256, MD5).
class PRIME_PUBLIC Hasher : public NonAtomicRefCounted<Hasher> {
public:
    virtual ~Hasher();

    virtual void reset() PRIME_NOEXCEPT = 0;

    virtual void process(const void* bytes, size_t byteCount) PRIME_NOEXCEPT = 0;

    virtual std::vector<uint8_t> get() = 0;
};

/// e.g., HasherWrapper<SHA256> make SHA256 implement Hasher
template <typename Algorithm>
class HasherWrapper : public Hasher {
public:
    virtual void reset() PRIME_NOEXCEPT
    {
        _hasher.reset();
    }

    virtual void process(const void* bytes, size_t byteCount) PRIME_NOEXCEPT
    {
        _hasher.process(bytes, byteCount);
    }

    virtual std::vector<uint8_t> get()
    {
        std::vector<uint8_t> v;
        v.resize(Algorithm::digestSize);
        memcpy(&v[0], _hasher.getBytes().data(), Algorithm::digestSize);
        return v;
    }

private:
    Algorithm _hasher;
};
}

#endif
