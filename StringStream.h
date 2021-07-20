// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STRINGSTREAM_H
#define PRIME_STRINGSTREAM_H

#include "Optional.h"
#include "Stream.h"
#include "StringView.h"
#include <string>

namespace Prime {

/// A read/write in-memory Stream which dynamically resizes as necessary and where the accumulated data can be
/// accessed as a null-terminated string. If you only need to read, use a StreamBuffer, which can be given a
/// buffer to read from without copying it.
class PRIME_PUBLIC StringStream : public Stream {
    PRIME_DECLARE_UID_CAST(Stream, 0xdb08d25c, 0x4c3746bd, 0x9c8f67e6, 0x7c8f4333)

public:
    StringStream()
        : _offset(0)
    {
    }

    explicit StringStream(size_t initialOffset)
        : _offset(initialOffset)
    {
    }

    explicit StringStream(StringView string, size_t initialOffset = 0)
        : _string(string.begin(), string.end())
        , _offset(initialOffset)
    {
    }

    /// Reserve memory for a stream of the specified size.
    void reserve(size_t bytes);

    /// Set a maximum size. Once this size is reached, content will begin being removed from the middle of the
    /// buffer. This is a slow, cache trashing process, so set extraBytesToTrim as high as possible to reduce
    /// the frequence it needs to happen.
    void setMaxSize(size_t maxSizeInBytes, size_t extraBytesToTrim);

    /// Empty the stream and reset the file offset to zero.
    void clear();

    /// Get the data as raw bytes.
    const void* getBytes() const { return _string.data(); }

    /// Get the size of the data.
    size_t getSize() const { return _string.size(); }

    /// Direct access to the std::string. The string can be modified.
    std::string& getString() { return _string; }

    /// Direct access to the std::string.
    const std::string& getString() const { return _string; }

    /// Get the data as a C string (i.e., null terminated).
    const char* c_str() const { return _string.c_str(); }

    typedef char* iterator;
    typedef const char* const_iterator;

    const char* begin() const { return _string.data(); }

    char* begin() { return _string.empty() ? (char*)NULL : &_string[0]; }

    const char* end() const { return _string.data() + _string.size(); }

    char* end() { return begin() + _string.size(); }

    /// Set the buffer size and copy the supplied bytes in to it. If you're going to read but not write then you
    /// should use a StreamBuffer instead.
    void setBytes(const void* bytes, size_t size);

    // Stream implementation.
    virtual ptrdiff_t readSome(void* buffer, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeSome(const void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE;
    virtual Offset getSize(Log* log) PRIME_OVERRIDE;
    virtual bool setSize(Offset size, Log* log) PRIME_OVERRIDE;

private:
    std::string _string;
    size_t _offset;
    Optional<size_t> _maxSizeInBytes;
    size_t _extraBytesToTrim;

    PRIME_UNCOPYABLE(StringStream);
};

}

#endif
