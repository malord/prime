// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PROPERTYLISTWRITER_H
#define PRIME_PROPERTYLISTWRITER_H

#include "Log.h"
#include "PropertyListReader.h" // Declares the PropertyListFormat enum
#include "Stream.h"
#include "Value.h"

namespace Prime {

class PRIME_PUBLIC PropertyListWriter {
public:
    enum { defaultBufferSize = 8192u };

    struct PRIME_PUBLIC Options {

        explicit Options(PropertyListFormat format = PropertyListFormatBinary)
            : _format(format)
            , _ascii(false)
        {
        }

        Options& setFormat(PropertyListFormat value)
        {
            _format = value;
            return *this;
        }
        PropertyListFormat getFormat() const { return _format; }

        Options& setASCII(bool value = true)
        {
            _ascii = value;
            return *this;
        }
        bool getASCII() const { return _ascii; }

    private:
        PropertyListFormat _format;
        bool _ascii;
    };

    /// Writes a property list to a Stream in the requested PropertyListFormat. If buffer is null, allocates a
    /// buffer of the specified size, otherwise the supplied buffer is used.
    bool write(Stream* stream, Log* log, const Value& value, const Options& options,
        size_t bufferSize = defaultBufferSize, void* buffer = NULL);
};
}

#endif
