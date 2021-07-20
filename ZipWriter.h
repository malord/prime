// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ZIPWRITER_H
#define PRIME_ZIPWRITER_H

#include "ZipFormat.h"

#ifndef PRIME_NO_ZLIB

#include "ScopedPtr.h"
#include "Stream.h"
#ifndef PRIME_CXX11_STL
#include "Callback.h"
#endif
#include <functional>
#include <vector>

#define PRIME_HAVE_ZIPWRITER

namespace Prime {

/// Uses deflate compression and ensures files are not "compressed" to larger than their original size.
class PRIME_PUBLIC ZipWriter {
public:
    ZipWriter();

    ~ZipWriter();

    class PRIME_PUBLIC Options {
    public:
        Options(int compressionLevel = 6)
            : _compressionLevel(compressionLevel)
            , _copyBufferSize(PRIME_HUGE_BUFFER_SIZE)
            , _deflateBufferSize(32768u)
        {
        }

        Options& setCompressionLevel(int value)
        {
            _compressionLevel = value;
            return *this;
        }
        int getCompressionLevel() const { return _compressionLevel; }

        Options& setCopyBufferSize(size_t value)
        {
            _copyBufferSize = value;
            return *this;
        }
        size_t getCopyBufferSize() const { return _copyBufferSize; }

        Options& setDeflateBufferSize(size_t value)
        {
            _deflateBufferSize = value;
            return *this;
        }
        size_t getDeflateBufferSize() const { return _deflateBufferSize; }

    private:
        int _compressionLevel;
        size_t _copyBufferSize;
        size_t _deflateBufferSize;

        friend class ZipWriter;
    };

    bool begin(Stream* stream, Log* log, const Options& options);

    bool beginFile(const Zip::CentralDirectoryEntry& partialCentralDirectoryEntry);

#ifdef PRIME_CXX11_STL
    typedef std::function<void(Stream::Offset /*compressedSoFar*/, Stream::Offset /*totalToCompress*/)> CompressionCallback;
#else
    typedef Callback2<void, Stream::Offset /*compressedSoFar*/, Stream::Offset /*totalToCompress*/> CompressionCallback;
#endif

    bool compressFileAndComputeCRC32(Stream* source, uint32_t& compressedSizeOut, uint32_t& crc32Out,
        uint16_t& methodOut, CompressionCallback compressionCallback = CompressionCallback());

    bool endFile(const Zip::CentralDirectoryEntry& cent, const char* filename, const char* extra, const char* comment);

    bool end();

private:
    bool writeCentralDirectory();

    bool writeEnd();

    bool copyBytesAcrossStreams(Stream* dest, Stream* source, uint64_t bytesToCopy);

    RefPtr<Stream> _stream;
    RefPtr<Log> _log;
    Options _options;
    ScopedArrayPtr<char> _copyBuffer;
    Stream::Offset _lentOffset;
    std::vector<char> _centralDirectory;
    Stream::Offset _centralDirectoryOffset;
    unsigned int _fileCount;
    CompressionCallback _compressionCallback;
};

}

#endif // PRIME_NO_ZLIB

#endif
