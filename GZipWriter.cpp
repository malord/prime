// Copyright 2000-2021 Mark H. P. Lord

#include "GZipWriter.h"

#ifdef PRIME_HAVE_GZIPWRITER

#include "GZipFormat.h"

namespace Prime {

using namespace GZip;

GZipWriter::GZipWriter()
{
    _begun = false;
}

GZipWriter::~GZipWriter()
{
    end(Log::getNullLog());
}

bool GZipWriter::begin(Stream* underlyingStream, int compressionLevel, Log* log)
{
    PRIME_ASSERT(!_begun);

    _underlyingStream = underlyingStream;

    Header header;
    char headerBytes[Header::defaultEncodedSize];
    header.encode(headerBytes);

    if (!_underlyingStream->writeExact(headerBytes, sizeof(headerBytes), log)) {
        return false;
    }

    _deflater.init(_underlyingStream, log);
    _deflater.setCompressionLevel(compressionLevel);

    _crcer.setStream(&_deflater);

    _bytesWritten = 0;
    _begun = true;

    return true;
}

ptrdiff_t GZipWriter::writeSome(const void* bytes, size_t maxBytes, Log* log)
{
    PRIME_ASSERT(_begun);

    ptrdiff_t wrote = _crcer.writeSome(bytes, maxBytes, log);
    if (wrote < 0) {
        return wrote;
    }

    _bytesWritten += wrote;
    return wrote;
}

bool GZipWriter::end(Log* log)
{
    if (!_begun) {
        return true;
    }

    _begun = false;

    if (!_deflater.end(log)) {
        return false;
    }

    Footer footer;
    footer.crc32 = _crcer.getHash();
    footer.originalSize = (uint32_t)_bytesWritten;

    char footerBytes[Footer::encodedSize];
    footer.encode(footerBytes);
    if (!_underlyingStream->writeExact(footerBytes, sizeof(footerBytes), log)) {
        return false;
    }

    return true;
}

bool GZipWriter::close(Log* log)
{
    bool success = end(log);

    return _crcer.close(log) && success;
}
}

#endif // PRIME_HAVE_GZIPWRITER
