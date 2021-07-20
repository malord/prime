// Copyright 2000-2021 Mark H. P. Lord

#include "PropertyListWriter.h"
#include "BinaryPropertyListWriter.h"
#include "JSONWriter.h"
#include "XMLPropertyListWriter.h"

namespace Prime {

bool PropertyListWriter::write(Stream* stream, Log* log, const Value& value, const Options& options, size_t bufferSize, void* buffer)
{
    if (!PRIME_GUARD(!value.isUndefined())) {
        return false;
    }

    switch (options.getFormat()) {
    case PropertyListFormatXML:
        return XMLPropertyListWriter().write(stream, log, value, XMLPropertyListWriter::Options(), bufferSize, buffer);

    case PropertyListFormatBinary:
        return BinaryPropertyListWriter().write(stream, log, value, BinaryPropertyListWriter::Options(), bufferSize, buffer);

    case PropertyListFormatJSON:
        return JSONWriter().write(stream, log, value,
            JSONWriter::Options().setUTF8(!options.getASCII()),
            bufferSize, buffer);

    default:
        PRIME_ASSERT(0);
        return false;
    }
}
}
