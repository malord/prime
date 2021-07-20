// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STREAMLOADER_H
#define PRIME_STREAMLOADER_H

#include "ScopedPtr.h"
#include "Stream.h"

namespace Prime {

/// Loads the entire contents of a Stream in to memory (in to a std::string) and frees it when destructed.
/// The Stream does not need to implement getSize(), but memory allocation is optimal if it does.
class PRIME_PUBLIC StreamLoader : public RefCounted {
public:
    StreamLoader()
        : _loaded(false)
    {
    }

    /// Load the remaining contents of a Stream in to an internal buffer.
    bool load(Stream* stream, Log* log);

    /// Free the loaded data and reset the size to zero.
    void reset();

    /// Returns true if something has been loaded.
    bool isLoaded() const { return _loaded; }

    /// Returns true if we haven't loaded anything.
    bool operator!() const { return !_loaded; }

    /// Returns a pointer to the bytes that were loaded.
    char* getBytes() const { return _string.empty() ? (char*)NULL : &_string[0]; }

    /// Returns the number of bytes that were loaded.
    size_t getSize() const { return _string.size(); }

    char* begin() const { return getBytes(); }
    char* end() const { return getBytes() + getSize(); }

    const std::string& getString() const { return _string; }
    std::string& getString() { return _string; }

    const char* c_str() const { return _string.c_str(); }

private:
    bool loadSizeUnknown(Stream* stream, Log* log);

    mutable std::string _string;
    bool _loaded;

    PRIME_UNCOPYABLE(StreamLoader);
};
}

#endif
