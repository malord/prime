// Copyright 2000-2021 Mark H. P. Lord

#include "ChunkedReader.h"
#include "NumberParsing.h"
#include "NumberUtils.h"
#include "StringUtils.h"

namespace Prime {

PRIME_DEFINE_UID_CAST(ChunkedReader)

ChunkedReader::ChunkedReader()
{
    construct();
}

ChunkedReader::ChunkedReader(StreamBuffer* buffer)
{
    construct();
    begin(buffer);
}

ChunkedReader::~ChunkedReader()
{
}

void ChunkedReader::construct()
{
    _chunkRemaining = 0;
    _finished = true;
}

void ChunkedReader::begin(StreamBuffer* buffer)
{
    construct();

    _buffer = buffer;

    _finished = false;
}

bool ChunkedReader::readChunkSize(Log* log)
{
    ptrdiff_t lineLength = _buffer->find("\r\n", 2, log);

    if (lineLength == 0) {
        // This skips the CRLF that trail each chunk.

        if (!_buffer->skipBytes(2, log)) {
            return false;
        }

        lineLength = _buffer->find("\r\n", 2, log);
    }

    if (lineLength < 0 || lineLength > 8) {
        return false;
    }

    const char* newPtr;
    if (!ParseInt(StringView((const char*)_buffer->getReadPointer(), (size_t)lineLength), newPtr, _chunkRemaining, 16)) {
        return false;
    }

    if (_chunkRemaining < 0) {
        return false;
    }

    newPtr = ASCIISkipSpacesAndTabs(newPtr);
    if (newPtr != (const char*)_buffer->getReadPointer() + (size_t)lineLength && *newPtr != ';') {
        return false;
    }

    if (!_buffer->skipBytes(lineLength + 2, log)) {
        return false;
    }

    if (_chunkRemaining == 0) {
        _finished = true;

        if (_buffer->matchBytes("\r\n", 2, log)) {
            if (!_buffer->skipBytes(2, log)) {
                return false;
            }
        }
    }

    return true;
}

bool ChunkedReader::hasFinished(Log* log)
{
    if (_finished) {
        return true;
    }

    if (_chunkRemaining) {
        return false;
    }

    if (!readChunkSize(log)) {
        return false;
    }

    return _finished;
}

ptrdiff_t ChunkedReader::readSome(void* buffer, size_t maximumBytes, Log* log)
{
    if (_chunkRemaining <= 0) {
        if (_finished) {
            return 0;
        }

        if (!readChunkSize(log)) {
            log->error(PRIME_LOCALISE("Invalid chunked encoding."));
            return -1;
        }

        if (_chunkRemaining == 0) {
            return 0;
        }
    }

    PRIME_ASSERT(_chunkRemaining > 0);

    size_t fetch = Min(maximumBytes, (size_t)_chunkRemaining);
    ptrdiff_t fetched = _buffer->readSome(buffer, fetch, log);
    if (fetched < 0) {
        return fetched;
    }

    _chunkRemaining -= fetched;
    return fetched;
}

bool ChunkedReader::close(Log* log)
{
    return _buffer->close(log);
}
}
