// Copyright 2000-2021 Mark H. P. Lord

#include "IconvReader.h"

#ifdef PRIME_HAVE_ICONVREADER

#include "StringUtils.h"
#include <errno.h>

namespace Prime {

static const std::string xmlWhitespace("\x20\x09\x0a\x0d\x0c");

const char* IconvReader::readBOM(Log* log)
{
    // Try to get four bytes in to the buffer. Usually, we'll get a lot more than this. Then see if there's a
    // Byte Order Mark to help us out, and if there is, skip it too.

    ptrdiff_t available = _streamBuffer.requestNumberOfBytes(4, log);

    if (available < 0) {
        return NULL;
    }

    const uint8_t* readPtr = (const unsigned char*)_streamBuffer.getReadPointer();

    if (available >= 3 && readPtr[0] == 0xef && readPtr[1] == 0xbb && readPtr[2] == 0xbf) {
        _streamBuffer.advanceReadPointer(3);
        return "UTF-8";
    }

    if (available >= 4 && readPtr[0] == 0 && readPtr[1] == 0 && readPtr[2] == 0xfe && readPtr[3] == 0xff) {
        _streamBuffer.advanceReadPointer(4);
        return "UTF-32BE";
    }

    if (available >= 4 && readPtr[0] == 0xff && readPtr[1] == 0xfe && readPtr[2] == 0x00 && readPtr[3] == 0x00) {
        _streamBuffer.advanceReadPointer(4);
        return "UTF-32LE";
    }

    if (available >= 2 && readPtr[0] == 0xfe && readPtr[1] == 0xff) {
        _streamBuffer.advanceReadPointer(2);
        return "UTF-16BE";
    }

    if (available >= 2 && readPtr[0] == 0xff && readPtr[1] == 0xfe) {
        _streamBuffer.advanceReadPointer(2);
        return "UTF-16LE";
    }

    return "";
}

const char* IconvReader::guessEncodingWhereFirstCharacterIsASCII(Log* log)
{
    const char* bom = readBOM(log);
    if (!bom) {
        return NULL;
    }

    if (*bom) {
        return bom;
    }

    ptrdiff_t available = _streamBuffer.requestNumberOfBytes(4, log);

    if (available < 0) {
        return NULL;
    }

    const uint8_t* readPtr = (const uint8_t*)_streamBuffer.getReadPointer();

    if (available >= 4 && readPtr[0] == 0 && readPtr[1] == 0 && (readPtr[2] || readPtr[3])) {
        return "UTF-32BE";
    }

    if (available >= 4 && (readPtr[0] || readPtr[1]) && readPtr[2] == 0 && readPtr[3] == 0) {
        return "UTF-32LE";
    }

    if (available >= 2 && readPtr[0] && readPtr[1] == 0) {
        return "UTF-16LE";
    }

    if (available >= 2 && readPtr[0] == 0 && readPtr[1]) {
        return "UTF-16BE";
    }

    return "";
}

bool IconvReader::guessEncoding(char* buffer, size_t bufferSize, bool shouldFirstCharacterBeASCII, bool* isXML, Log* log)
{
    PRIME_ASSERT(_streamBuffer.getBufferSize() >= 4);
    PRIME_ASSERT(_streamBuffer.getUnderlyingStream());

    if (isXML) {
        shouldFirstCharacterBeASCII = true;
    }

    const char* guess;

    if (shouldFirstCharacterBeASCII) {
        // guessEncodingWhereFirstCharacterIsASCII() tries readBOM() first.
        guess = guessEncodingWhereFirstCharacterIsASCII(log);
    } else {
        guess = readBOM(log);
    }

    if (!guess) {
        return false;
    }

    StringCopy(buffer, bufferSize, guess);

    if (isXML == NULL) {
        // Not parsing an XML file, so that's all we can do.
        return true;
    }

    *isXML = false;

    // Load our _streamBuffer as much as we possibly can.
    if (_streamBuffer.fetchUntilBufferIsFull(log) < 0) {
        return false; // Read error.
    }

    // Create another IconvReader set up to read from the bytes we have in our buffer using the encoding we think
    // we have, falling back to ISO-8859-1. This allows us to read from the underlying stream without having to
    // seek to go back afterwards.
    IconvReader partIconvReader(_streamBuffer.getReadPointer(), _streamBuffer.getBytesAvailable());
    if (!partIconvReader.beginIconv("UTF-8", *guess ? guess : "ISO-8859-1")) {
        // Possibly just an unsupported format.
        return true;
    }

    // Buffer reads from the partIconvReader.
    char partStreamBufferBuffer[512];
    StreamBuffer partStreamBuffer(&partIconvReader, sizeof(partStreamBufferBuffer), partStreamBufferBuffer);

    // Skip whitespace.
    if (!partStreamBuffer.skipMatchingBytes(true, true, xmlWhitespace, log)) {
        return false;
    }

    // If it's XML, we'll now have a <.
    *isXML = partStreamBuffer.peekByte(log) == '<';

    // If we've got a guess then don't read any more - respect the BOM.
    if (*guess) {
        return true;
    }

    // If we haven't got an XML declaration, we can't do any better with our guess.
    static const char xmlDecl[] = "<?xml";
    for (size_t i = 0; xmlDecl[i]; ++i) {
        int c = partStreamBuffer.peekByte(i, log);
        if (c < 0) {
            return !partStreamBuffer.getErrorFlag();
        }
        if (c != xmlDecl[i]) {
            return true;
        }
    }

    // Got <?xml, now look for encoding=
    partStreamBuffer.advanceReadPointer(COUNTOF(xmlDecl) - 1);

    static const char encodingAttrPrefix[] = "encoding";

    for (;;) {
    find_encoding:
        if (!partStreamBuffer.skipMatchingBytes(true, true, xmlWhitespace, log)) {
            return false;
        }

        // See if we match "encoding"
        for (size_t i = 0; encodingAttrPrefix[i]; ++i) {
            int c = partStreamBuffer.peekByte(i, log);
            if (c < 0) {
                return !partStreamBuffer.getErrorFlag();
            }
            if (c != encodingAttrPrefix[i]) {
                if (c == '>') {
                    return true;
                }

                partStreamBuffer.skipByte();
                goto find_encoding;
            }
        }

        break;
    }

    partStreamBuffer.advanceReadPointer(sizeof(encodingAttrPrefix) - 1);

    if (!partStreamBuffer.skipMatchingBytes(true, true, xmlWhitespace, log)) {
        return false;
    }

    if (partStreamBuffer.readByte(log) != '=') {
        return true;
    }

    if (!partStreamBuffer.skipMatchingBytes(true, true, xmlWhitespace, log)) {
        return false;
    }

    int quot = partStreamBuffer.readByte(log);
    if (quot != '\'' && quot != '"') {
        return true;
    }

    // We have our encoding.
    char* outPtr = buffer;
    char* outEnd = buffer + bufferSize - 1;

    for (;;) {
        int c = partStreamBuffer.readByte(log);

        if (c < 0) {
            buffer[0] = 0;
            return !partStreamBuffer.getErrorFlag();
        }

        if (c == quot) {
            break;
        }

        if (outPtr == outEnd) {
            break;
        }

        *outPtr++ = (char)(unsigned char)c;
    }

    *outPtr = 0;
    PRIME_ASSERT(*isXML);

    return true;
}

bool IconvReader::beginIconv(const char* toEncoding, const char* fromEncoding, bool force, const IconvWrapper::Options& options)
{
    _passThrough = false;

    if (!force && StringsEqual(toEncoding, fromEncoding)) {
        _passThrough = true;
        return true;
    }

    return _iconv.open(toEncoding, fromEncoding, options);
}

bool IconvReader::close(Log* log)
{
    _passThrough = true;
    _iconv.close();

    return _streamBuffer.close(log);
}

ptrdiff_t IconvReader::readSome(void* memory, size_t maxBytes, Log* log)
{
    if (_passThrough) {
        return _streamBuffer.readSome(memory, maxBytes, log);
    }

    for (;;) {
        const char* inBuffer = (const char*)_streamBuffer.getReadPointer();
        size_t inBytesLeft = _streamBuffer.getBytesAvailable();
        char* outBuffer = (char*)memory;
        size_t outBytesLeft = maxBytes;

        if (inBytesLeft) {
            if (!_iconv.iconv(&inBuffer, inBytesLeft, &outBuffer, outBytesLeft, log)) {
                return -1;
            }

            _streamBuffer.setReadPointer((const uint8_t*)inBuffer);

            if (outBuffer > (char*)memory) {
                return outBuffer - (char*)memory;
            }
        }

        ptrdiff_t fetched = _streamBuffer.fetchMore(log);
        if (fetched < 0) {
            return -1;
        }
        if (fetched == 0) {
            return 0;
        }
    }
}
}

#endif // PRIME_HAVE_ICONVREADER
