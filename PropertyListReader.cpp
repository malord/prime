// Copyright 2000-2021 Mark H. P. Lord

#include "PropertyListReader.h"
#include "BinaryPropertyListReader.h"
#include "IconvReader.h"
#include "JSONReader.h"
#include "StringUtils.h"
#include "XMLPropertyListReader.h"

namespace Prime {

Value PropertyListReader::read(Stream* stream, Log* log, PropertyListFormat* formatOut)
{
    // We need to peek in to the stream to check the header. If the supplied Stream is a StreamBuffer, we
    // can use that directly. Otherwise, we need to create our own StreamBuffer.
    StreamBuffer* streamBuffer;
    StreamBuffer tempStreamBuffer;
    streamBuffer = UIDCast<StreamBuffer>(stream);
    if (!streamBuffer) {
        tempStreamBuffer.init(stream, PRIME_FILE_BUFFER_SIZE);
        streamBuffer = &tempStreamBuffer;
    }

    // Try for binary (which has a specific header we can check for). Apple's libraries write empty files for
    // empty dictionaries, so deal with that first.
    ptrdiff_t gotFirst6Bytes = streamBuffer->requestNumberOfBytes(6, log);
    if (gotFirst6Bytes == 0) {
        log->verbose(PRIME_LOCALISE("Empty property list file."));
        return Value::Dictionary();
    }
    if (gotFirst6Bytes < 0) {
        return undefined;
    }
    if (gotFirst6Bytes >= 6 && memcmp(streamBuffer->getReadPointer(), "bplist", 6) == 0) {
        if (formatOut) {
            *formatOut = PropertyListFormatBinary;
        }

        BinaryPropertyListReader binaryReader;
        return binaryReader.read(streamBuffer, log);
    }

// It's a text file.
#ifdef PRIME_HAVE_ICONVREADER

    IconvReader iconvReader(streamBuffer, 512);

    char encoding[128];
    bool isXML;
    if (!iconvReader.guessEncoding(encoding, sizeof(encoding), true, &isXML, log)) {
        return undefined;
    }

    // isXML is set to true if the first non-XML-whitespace character was a '<', which is what we need to
    // determine XML vs. JSON.

    PropertyListFormat format = isXML ? PropertyListFormatXML : PropertyListFormatJSON;

    // If the encoding couldn't be determined, go with UTF-8.
    if (!encoding[0]) {
        StringCopy(encoding, sizeof(encoding), "UTF-8");
    }

    if (!iconvReader.beginIconv("UTF-8", encoding)) {
        log->error(PRIME_LOCALISE("Can't convert %s to UTF-8"), encoding);
        return undefined;
    }

    if (formatOut) {
        *formatOut = format;
    }

    switch (format) {
    case PropertyListFormatJSON: {
        JSONReader jsonReader;
        return jsonReader.read(&iconvReader, log);
    }

    case PropertyListFormatXML: {
        XMLPropertyListReader xmlReader;
        return xmlReader.read(&iconvReader, log);
    }

    default:
        PRIME_ASSERT(0);
        return undefined;
    }

#else // PRIME_HAVE_ICONVREADER

    PropertyListFormat format;

    for (;;) {
        const char* ptr = streamBuffer->getReadPointer();
        const char* top = streamBuffer->getTopPointer();
        for (; ptr != top; ++ptr) {
            if (*ptr == '<') {
                format = PropertyListFormatXML;
                goto found_format;
            }
            if (*ptr > ' ') {
                format = PropertyListFormatJSON;
                goto found_format;
            }
        }

        // Discard the whitespace.
        streamBuffer->setReadPointer(streamBuffer->getTopPointer());

        ptrdiff_t fetched = streamBuffer->fetchMore(log);
        if (fetched < 0) {
            return undefined;
        }
        if (fetched == 0) {
            log->error(PRIME_LOCALISE("Empty property list."));
            return undefined;
        }
    }

found_format:

    if (formatOut) {
        *formatOut = format;
    }

    switch (format) {
    case PropertyListFormatJSON: {
        JSONReader jsonReader;
        return jsonReader.read(streamBuffer, log);
    }

    case PropertyListFormatXML: {
        XMLPropertyListReader xmlReader;
        return xmlReader.read(streamBuffer, log);
    }

    default:
        PRIME_ASSERT(0);
        return undefined;
    }

#endif // PRIME_HAVE_ICONVREADER
}
}
