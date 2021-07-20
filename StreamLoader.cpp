// Copyright 2000-2021 Mark H. P. Lord

#include "StreamLoader.h"
#include <string.h>

namespace Prime {

void StreamLoader::reset()
{
    _loaded = false;
    _string.resize(0);
#ifdef PRIME_CXX11_STL
    _string.shrink_to_fit();
#endif
}

bool StreamLoader::load(Stream* stream, Log* log)
{
    reset();

    if (!PRIME_GUARD(stream)) {
        return false;
    }

    Stream::Offset streamSize;

    if (!stream->isSeekable() || (streamSize = stream->getSize(Log::getNullLog())) < 0) {
        return loadSizeUnknown(stream, log);
    }

    Stream::Offset streamOffset = stream->getOffset(log);
    if (streamOffset < 0) {
        return false;
    }

    size_t size = (size_t)(streamSize - streamOffset);

    if ((Stream::Offset)size != (streamSize - streamOffset)) {
        log->error(PRIME_LOCALISE("File too large to load (exceeds addressable memory)."));
        return false;
    }

    _string.resize(size);

    if (size && !stream->readExact(&_string[0], size, log)) {
        reset();
        return false;
    }

    return true;
}

bool StreamLoader::loadSizeUnknown(Stream* stream, Log* log)
{
    reset();

    for (;;) {
        char buffer[PRIME_BIG_STACK_BUFFER_SIZE];

        ptrdiff_t bytesRead = stream->readSome(buffer, sizeof(buffer), log);

        if (bytesRead < 0) {
            return false;
        }

        if (bytesRead == 0) {
            break;
        }

        _string.append(buffer, buffer + bytesRead);
    }

    return true;
}
}
