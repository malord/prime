// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ICONVREADER_H
#define PRIME_ICONVREADER_H

#include "IconvWrapper.h"

#ifdef PRIME_HAVE_ICONVWRAPPER

#include "StreamBuffer.h"

#define PRIME_HAVE_ICONVREADER

namespace Prime {

/// A Stream for character files which converts between character encodings. Capable of detecting (or guessing)
/// the source encoding from the file, with specific support for XML. See PropertyListReader.cpp for an example.
class PRIME_PUBLIC IconvReader : public Stream {
public:
    IconvReader()
        : _passThrough(true)
    {
    }

    /// Allocate a buffer of the specified size and optionally assign the underlying Stream to buffer.
    IconvReader(Stream* stream, size_t bufferSize, void* buffer = NULL)
        : _streamBuffer(stream, bufferSize, buffer)
        , _passThrough(true)
    {
    }

    /// Read from the supplied array of bytes, rather than a Stream.
    IconvReader(const void* bytes, size_t byteCount)
        : _streamBuffer(bytes, byteCount)
        , _passThrough(true)
    {
    }

    /// Read a Byte Order Mark from the file. Returns a null pointer on error, an empty string if there was no
    /// BOM, or a pointer to the encoding name if there is a BOM. (e.g., returns "UTF-16LE" if that BOM is found).
    const char* readBOM(Log* log);

    /// Attempt to guess the encoding for a file format where the first character is ASCII. Returns a null
    /// pointer on error, an empty string if the file format cannot be guessed, or a pointer to the encoding
    /// name if a guess was made. Tries readBOM() first.
    const char* guessEncodingWhereFirstCharacterIsASCII(Log* log);

    /// Attempt to determine the encoding of a text file, which may or may not be an XML file. Returns false
    /// if a read error occurs, otherwise returns true and writes the encoding name in to buffer. If the encoding
    /// cannot be determined, an empty string is written to buffer (and true is still returned). If isXML is
    /// non-null, the method skips XML whitespace and sets *isXML to true if the first non-whitespace character
    /// is '<' and false if it's not. If the file is determined to be XML, and if (and only if) the encoding is
    /// an 8-bit encoding, the file encoding is then read from the <?xml version=... encoding=...> declaration.
    /// For this to work, the XML declaration must fit within the buffer size.
    bool guessEncoding(char* buffer, size_t bufferSize, bool shouldFirstCharacterBeASCII, bool* isXML, Log* log);

    /// Begin conversion. The readSome() method (and therefore read(), readExact(), etc.) will read through iconv.
    bool beginIconv(const char* toEncoding, const char* fromEncoding, bool force = false,
        const IconvWrapper::Options& options = IconvWrapper::Options());

    //
    // Stream implementation
    //

    virtual bool close(Log* log) PRIME_OVERRIDE;

    /// Read characters via iconv. Returns < 0 on error, otherwise returns the number of bytes (not characters)
    /// decoded.
    virtual ptrdiff_t readSome(void* memory, size_t maxBytes, Log* log) PRIME_OVERRIDE;

private:
    StreamBuffer _streamBuffer;
    IconvWrapper _iconv;
    bool _passThrough;

    PRIME_UNCOPYABLE(IconvReader);
};
}

#endif // PRIME_HAVE_ICONVWRAPPER

#endif
