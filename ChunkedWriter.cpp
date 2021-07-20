// Copyright 2000-2021 Mark H. P. Lord

#include "ChunkedWriter.h"

namespace Prime {

ChunkedWriter::ChunkedWriter()
{
    construct();
}

ChunkedWriter::ChunkedWriter(Stream* stream)
{
    construct();
    begin(stream);
}

ChunkedWriter::~ChunkedWriter()
{
    end(Log::getNullLog());
}

void ChunkedWriter::construct()
{
    _bytesWritten = 0;
    _needEndWrite = false;
}

void ChunkedWriter::begin(Stream* stream)
{
    construct();

    _stream = stream;
}

ptrdiff_t ChunkedWriter::writeSome(const void* memory, size_t maximumBytes, Log* log)
{
    // This says we're at the end, so don't write this.
    if (maximumBytes == 0) {
        return 0;
    }

    _needEndWrite = true;

    if (!_stream->printf(log, "%" PRIxPTR "\r\n", maximumBytes)) {
        return -1;
    }
    if (!_stream->writeExact(memory, maximumBytes, log)) {
        return -1;
    }
    if (!_stream->printf(log, "\r\n")) {
        return -1;
    }

    _bytesWritten += maximumBytes;

    return (ptrdiff_t)maximumBytes;
}

bool ChunkedWriter::end(Log* log)
{
    if (!_needEndWrite) {
        return true;
    }

    _needEndWrite = false;

    return _stream->printf(log, "0\r\n\r\n");
}

bool ChunkedWriter::close(Log* log)
{
    if (!end(log)) {
        return false;
    }

    return _stream->close(log);
}
}
