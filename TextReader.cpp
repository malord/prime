// Copyright 2000-2021 Mark H. P. Lord

#include "TextReader.h"
#include "StringUtils.h"

namespace Prime {

namespace {
    static const char utf8BOM[] = "\xef\xbb\xbf";
}

//
// TextReader::LocationLog
//

TextReader::LocationLog::LocationLog()
    : _textReader(NULL)
{
}

bool TextReader::LocationLog::logVA(Level level, const char* format, va_list argptr)
{
    char stack[128];

    if (StringReplaceInPlace(stack, sizeof(stack), _textReader->getLocation(), "%", "%%") && StringAppend(stack, sizeof(stack), ": ", 2) && StringAppend(stack, sizeof(stack), format)) {
        return _textReader->_log->logVA(level, stack, argptr);
    }

    std::string temp;
    temp = _textReader->getLocation();
    StringReplaceInPlace(temp, "%", "%%");
    temp += ": ";
    temp += format;

    return _textReader->_log->logVA(level, temp.c_str(), argptr);
}

//
// TextReader
//

TextReader::TextReader()
{
    _locationLog.setTextReader(this);
    _ptr = _top = 0;
    _buffer = _bufferEnd = 0;
    _tokenStart = 0;
    _firstMarker = 0;
    _beginLocation.line = _beginLocation.column = 0;
    _beginLocation.previousChar = 0;
    _beginLocation.ptr = 0;
    _tokenLocation.ptr = 0;
    _streamCheckForBOM = false;
    _skipBOM = false;
}

TextReader::~TextReader()
{
}

void TextReader::setLocation(unsigned int line, unsigned int column)
{
    _beginLocation.line = line;
    _beginLocation.column = column;
    _beginLocation.previousChar = 0;
    _tokenLocation.ptr = 0;
}

void TextReader::setText(StringView source, unsigned int line, unsigned int column)
{
    if (_skipBOM && StringStartsWith(source, utf8BOM)) {
        // Skip UTF-8 BOM
        source = source.substr(3);
    }

    PRIME_ASSERT(!_firstMarker);
    _stream.release();
    _ptr = _beginLocation.ptr = source.begin();
    _top = source.end();
    _buffer = _bufferEnd = 0;
    _tokenLocation.ptr = 0;
    _tokenStart = 0;
    _errorState = ErrorStateNone;

    setLocation(line, column);
}

void TextReader::setStream(Stream* stream, size_t bufferSize, unsigned int line, unsigned int column)
{
    PRIME_ASSERT(!_firstMarker);
    _stream = stream;
    _streamCheckForBOM = _skipBOM;
    allocBuffer(bufferSize);
    _ptr = _top = _beginLocation.ptr = _buffer;
    _tokenLocation.ptr = 0;
    _tokenStart = 0;
    _errorState = ErrorStateNone;

    setLocation(line, column);
}

void TextReader::allocBuffer(size_t bufferSize)
{
    _dynamicBuffer.resize(bufferSize);
    _buffer = &_dynamicBuffer[0];
    _bufferEnd = _buffer + bufferSize;
}

unsigned int TextReader::getLine() const
{
    updateTokenLocation();
    return _tokenLocation.line;
}

unsigned int TextReader::getColumn() const
{
    updateTokenLocation();
    return _tokenLocation.column;
}

const char* TextReader::getLocation() const
{
    updateTokenLocation();

    StringFormat(_locationBuffer, sizeof(_locationBuffer), "(%u:%u)", _tokenLocation.line + 1, _tokenLocation.column + 1);

    return _locationBuffer;
}

bool TextReader::hasString(const char* string)
{
    return hasString(0, string);
}

bool TextReader::hasString(unsigned int at, const char* string)
{
    for (; string[at]; ++at) {
        if (peekChar(at) != string[at]) {
            return false;
        }
    }

    return true;
}

int TextReader::slowPeekChar(unsigned int at)
{
    for (;;) {
        ptrdiff_t bytesRead = fetchMore();

        if (bytesRead < 0) {
            return ErrorChar;
        }

        if (bytesRead == 0) {
            return EOFChar;
        }

        if (at < (unsigned int)(_top - _ptr)) {
            return charToInt(_ptr[at]);
        }
    }
}

ptrdiff_t TextReader::fetchMore()
{
    if (_errorState != ErrorStateNone) {
        return _errorState == ErrorStateEOF ? 0 : -1;
    }

    // If there's no stream, we can't read more.
    if (!_stream) {
        return 0;
    }

    // Find the lowest point in the buffer that is locked.
    const char* lowestMarker = _ptr;
    for (Marker* m = _firstMarker; m; m = m->_prevMarker) {
        if (m->_ptr < lowestMarker) {
            lowestMarker = m->_ptr;
        }
    }

    // Is a Marker locking the first byte in the buffer? (In which case all the bytes in the buffer are needed and
    // there's nothing we can purge to make room.)
    if (lowestMarker == _buffer && _top == _bufferEnd) {
        if (!growDynamicBuffer()) {
            return -1;
        }

        // _buffer will have changed.
        lowestMarker = _buffer;
    }

    // Scooch the buffer along, purging bytes we no longer need and making room at the end of the buffer.
    if (lowestMarker > _buffer) {
        size_t scooch = lowestMarker - _buffer;

        // Update line and column for the characters we're purging.
        PRIME_ASSERT(_beginLocation.ptr == _buffer);
        updateLocation(_beginLocation.ptr, lowestMarker, _beginLocation);
        PRIME_ASSERT(_beginLocation.ptr == lowestMarker);
        _beginLocation.ptr = _buffer;

        memmove(_buffer, lowestMarker, _top - lowestMarker);

        // Scooch some pointers.
        _ptr -= scooch;
        _top -= scooch;

        if (_tokenStart) {
            _tokenStart = (_tokenStart < lowestMarker) ? _buffer : _tokenStart - scooch;
        }

        // _tokenLocation is just a cache. Invalidate it.
        _tokenLocation.ptr = 0;

        // Scooch the markers along.
        for (Marker* m = _firstMarker; m; m = m->_prevMarker) {
            m->_ptr -= scooch;
        }
    }

    // Load some more bytes in to the buffer.
    ptrdiff_t got = _stream->readSome((char*)_top, _bufferEnd - _top, _log);
    if (got < 0) {
        _errorState = ErrorStateError;
        return -1;
    }

    if (got == 0) {
        _errorState = ErrorStateEOF;
    }

    _top += got;

    // This is a little bit broken but usually good enough. If we deal with a stream which gives, say, 1 byte
    // at a time, we'll miss the BOM, but this is preferable to forcibly reading 2 more bytes when the stream
    // may be a network stream where no more bytes are available, which would block the thread.
    if (_streamCheckForBOM) {
        _streamCheckForBOM = false;
        if (_top - _ptr >= 3 && memcmp(_ptr, utf8BOM, 3) == 0) {
            _ptr += 3;
            got -= 3;
            if (got == 0) {
                return fetchMore();
            }
        }
    }

    return got;
}

bool TextReader::growDynamicBuffer()
{
    if (!isDynamicBuffer()) {
        return false;
    }

    // TODO: set a maximum buffer size that shouldn't be exceeded!

    size_t newBufferSize = _dynamicBuffer.size() * 2;
    _dynamicBuffer.resize(newBufferSize);

    rebasePointers(_buffer, &_dynamicBuffer[0]);

    _buffer = &_dynamicBuffer[0];
    _bufferEnd = _buffer + newBufferSize;

    return true;
}

void TextReader::rebasePointers(char* from, char* to)
{
    for (Marker* m = _firstMarker; m; m = m->_prevMarker) {
        m->_ptr = m->_ptr - from + to;
    }

    _ptr = _ptr - from + to;
    _top = _top - from + to;

    if (_tokenStart) {
        _tokenStart = _tokenStart - from + to;
    }

    PRIME_ASSERT(_beginLocation.ptr);
    _beginLocation.ptr = _beginLocation.ptr - from + to;

    if (_tokenLocation.ptr) {
        _tokenLocation.ptr = _tokenLocation.ptr - from + to;
    }
}

void TextReader::updateLocation(const char* from, const char* to, Location& loc)
{
    for (const char* p = from; p != to; ++p) {
        if (*p == '\r') {
            if (loc.previousChar != '\n') {
                ++loc.line;
                loc.column = 0;

                if (p + 1 != to && p[1] == '\n') {
                    ++p;
                }
            }
        } else if (*p == '\n') {
            if (loc.previousChar != '\r') {
                ++loc.line;
                loc.column = 0;

                if (p + 1 != to && p[1] == '\r') {
                    ++p;
                }
            }
        } else {
            ++loc.column;
        }
    }

    if (to > from) {
        loc.previousChar = to[-1];
    }

    loc.ptr = to;
}

void TextReader::updateTokenLocation() const
{
    const char* to = _tokenStart ? _tokenStart : _ptr;

    // _tokenLocation.ptr can be > to if we've rewound to a marker, and will be zero if it's been
    // invalidate (e.g., because the buffer has been resized).
    if (!_tokenLocation.ptr || _tokenLocation.ptr > to) {
        _tokenLocation = _beginLocation;
    } else if (_tokenLocation.ptr == to) {
        return;
    }

    updateLocation(_tokenLocation.ptr, to, _tokenLocation);
}
}
