// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_XMLPROPERTYLISTREADER_H
#define PRIME_XMLPROPERTYLISTREADER_H

#include "Value.h"
#include "XMLPullParser.h"

namespace Prime {

/// Reads a property list from an XML file. Supports all XML documents that match Apple's PLIST DTD.
class PRIME_PUBLIC XMLPropertyListReader {
public:
    enum { defaultBufferSize = PRIME_FILE_BUFFER_SIZE };

    explicit XMLPropertyListReader();

    /// In order to support encodings other than UTF-8 you must supply an IconReader initialised with
    /// guessEncoding() (note that PropertyListReader does this for you). Returns an invalid Value on error.
    Value read(Stream* stream, Log* log, size_t bufferSize = defaultBufferSize);

    /// In order to support encodings other than UTF-8 you must supply an IconReader initialised with
    /// guessEncoding() (note that PropertyListReader does this for you). Returns an invalid Value on error.
    Value read(TextReader* textReader);

    /// This method can be used to read a property list from within an XML document. The reader should be called
    /// with the relevant element (e.g., "dict", "array", etc.) already read.
    Value readElement(XMLPullParser* xmlParser);

private:
    Value readBool(bool value);
    Value readInteger();
    Value readReal();
    Value readString();
    Value readDate();
    Value readData();
    Value readArray();
    Value readDict();

    XMLPullParser* _xmlParser;

    PRIME_UNCOPYABLE(XMLPropertyListReader);
};
}

#endif
