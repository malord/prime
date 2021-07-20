// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_FILELOADER_H
#define PRIME_FILELOADER_H

#include "StreamLoader.h"

#define PRIME_FILELOADER_SUPPORT_STDIN

namespace Prime {

/// Loads a file from the OS file system in to a std::string.
class PRIME_PUBLIC FileLoader {
public:
    FileLoader() PRIME_NOEXCEPT
    {
    }

    /// Load the contents of a file in to an internal buffer.
    bool load(const char* path, Log* log);

#ifdef PRIME_FILELOADER_SUPPORT_STDIN
    // If the path is "-", reads from stdin.
    bool loadSupportingStdin(const char* path, Log* log);
#endif

    /// Free the loaded data and reset the size to zero.
    void reset() PRIME_NOEXCEPT { _loader.reset(); }

    /// Returns true if something has been loaded.
    bool isLoaded() const PRIME_NOEXCEPT { return _loader.isLoaded(); }

    /// Returns true if we haven't loaded anything.
    bool operator!() const PRIME_NOEXCEPT { return _loader.operator!(); }

    /// Returns a pointer to the bytes that were loaded.
    char* getBytes() const PRIME_NOEXCEPT { return _loader.getBytes(); }

    /// Returns the number of bytes that were loaded.
    size_t getSize() const PRIME_NOEXCEPT { return _loader.getSize(); }

    char* begin() const PRIME_NOEXCEPT { return _loader.begin(); }
    char* end() const PRIME_NOEXCEPT { return _loader.end(); }

    const std::string& getString() const PRIME_NOEXCEPT { return _loader.getString(); }
    std::string& getString() PRIME_NOEXCEPT { return _loader.getString(); }

    const char* c_str() const PRIME_NOEXCEPT { return _loader.c_str(); }

private:
    StreamLoader _loader;

    PRIME_UNCOPYABLE(FileLoader);
};
}

#endif
