// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PROPERTYLISTREADER_H
#define PRIME_PROPERTYLISTREADER_H

#include "Log.h"
#include "Stream.h"
#include "Value.h"

namespace Prime {

enum PropertyListFormat {
    PropertyListFormatXML,
    PropertyListFormatJSON,
    PropertyListFormatBinary
};

/// This is a class in order to conform to a pattern of a class having a read() method.
class PRIME_PUBLIC PropertyListReader {
public:
    /// Reads a property list, automatically determining what format it is and using iconv for XML and JSON files.
    Value read(Stream* stream, Log* log, PropertyListFormat* formatOut = NULL);
};
}

#endif
