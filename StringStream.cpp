// Copyright 2000-2021 Mark H. P. Lord

#include "StringStream.h"
#include "NumberUtils.h"
#include "StringUtils.h"
#include <string.h>

namespace Prime {

PRIME_DEFINE_UID_CAST(StringStream)

void StringStream::reserve(size_t bytes)
{
    _string.reserve(bytes);
}

void StringStream::setMaxSize(size_t maxSizeInBytes, size_t extraBytesToTrim)
{
    _maxSizeInBytes = maxSizeInBytes;
    _extraBytesToTrim = std::min(extraBytesToTrim, maxSizeInBytes / 2);
}

void StringStream::clear()
{
    _string.resize(0);
    _offset = 0;
}

void StringStream::setBytes(const void* bytes, size_t size)
{
    _string.assign((const char*)bytes, (const char*)bytes + size);
}

ptrdiff_t StringStream::readSome(void* buffer, size_t maximumBytes, Log*)
{
    size_t length = _string.size();
    if (_offset > length) {
        return 0;
    }

    size_t available = length - _offset;
    size_t take = Min<size_t>(available, maximumBytes);

    memcpy(buffer, _string.data() + _offset, take);
    _offset += take;

    return take;
}

ptrdiff_t StringStream::writeSome(const void* memory, size_t maximumBytes, Log*)
{
    // A write of zero bytes at an offset beyond the end will resize the string.
    //
    size_t end = _offset + maximumBytes;

    if (end > _string.size()) {
        _string.resize(end);
    }

    if (maximumBytes) {
        memcpy(&_string[_offset], memory, maximumBytes);
        _offset += maximumBytes;
    }

    if (_maxSizeInBytes && _string.size() > *_maxSizeInBytes) {
        MiddleTruncateStringInPlace(_string, *_maxSizeInBytes - _extraBytesToTrim, "...");
        if (_offset > _string.size()) {
            _offset = _string.size();
        }
    }

    return maximumBytes;
}

Stream::Offset StringStream::seek(Offset offset, SeekMode mode, Log*)
{
    Offset newLocation;

    switch (mode) {
    case SeekModeRelative:
        newLocation = offset + _offset;
        break;

    case SeekModeRelativeToEnd:
        newLocation = offset + _string.size();
        break;

    default:
    case SeekModeAbsolute:
        newLocation = offset;
        break;
    }

    if (newLocation < 0) {
        newLocation = 0;
    }

    _offset = (size_t)newLocation;

    // If the offset was truncated then it's outside the addressable memory range.
    if (!PRIME_GUARD((Offset)_offset == newLocation)) {
        return -1;
    }

    return (Offset)_offset;
}

Stream::Offset StringStream::getSize(Log*)
{
    return _string.size();
}

bool StringStream::setSize(Offset newSize, Log* log)
{
    size_t truncatedSize = (size_t)newSize;
    if ((Offset)truncatedSize != newSize) {
        log->error(PRIME_LOCALISE("StringStream maximum capacity exceeded."));
        return false;
    }

    _string.resize(truncatedSize);
    return true;
}

}
