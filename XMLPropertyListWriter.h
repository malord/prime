// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_XMLPROPERTYLISTWRITER_H
#define PRIME_XMLPROPERTYLISTWRITER_H

#include "Value.h"
#include "XMLWriter.h"

namespace Prime {

/// Writes property lists in Apple's XML property list format.
class PRIME_PUBLIC XMLPropertyListWriter {
public:
    XMLPropertyListWriter() { }

    enum { defaultBufferSize = PRIME_FILE_BUFFER_SIZE };

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _encoding("UTF-8")
        {
        }

        Options& setEncoding(std::string value)
        {
            _encoding.swap(value);
            return *this;
        }
        const std::string& getEncoding() const { return _encoding; }

    private:
        std::string _encoding;
    };

    /// Write the property list to a Stream. Also writes the XML file header and DOCTYPE.
    bool write(Stream* stream, Log* log, const Value& value, const Options& options,
        size_t bufferSize = defaultBufferSize, void* buffer = NULL);

    /// Write a single property list value to an XMLWriter. This allows you to embed plists in your own XML formats.
    bool write(XMLWriter* writer, const Value& value);

private:
    bool writeData(XMLWriter* writer, const Data& data);
    bool writeArray(XMLWriter* writer, const Value::Vector& array);
    bool writeDictionary(XMLWriter* writer, const Value::Dictionary& dictionary);

    PRIME_UNCOPYABLE(XMLPropertyListWriter);
};
}

#endif
