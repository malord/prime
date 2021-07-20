// Copyright 2000-2021 Mark H. P. Lord

#include "CSVWriter.h"
#include "StringUtils.h"

namespace Prime {

size_t CSVWriter::escapeInQuotes(char* buffer, size_t bufferSize, StringView string)
{
    if (buffer && !PRIME_GUARD(bufferSize)) {
        buffer = NULL;
    }

    char* dest;
    size_t space;

    if (buffer) {
        dest = buffer;
        space = bufferSize - 1;
    } else {
        dest = NULL;
        space = 0;
    }

    if (space) {
        *dest = '"';
        --space;
    }
    ++dest;

    const char* chars = string.begin();
    const char* charsEnd = string.end();

    for (;;) {
        const char* start = chars;
        while (chars != charsEnd && *chars != '"') {
            ++chars;
        }

        size_t got = chars - start;
        if (space && got) {
            memcpy(dest, start, PRIME_MIN(got, space));
        }
        if (got >= space) {
            space = 0;
        } else {
            space -= got;
        }
        dest += got;

        if (chars == charsEnd) {
            break;
        }

        ++chars;

        for (int quote = 0; quote != 2; ++quote) {
            if (space) {
                *dest = '"';
                --space;
            }
            ++dest;
        }
    }

    if (space) {
        *dest = '"';
        --space;
    }
    ++dest;

    if (buffer) {
        if (space) {
            *dest = 0;
        } else {
            buffer[bufferSize - 1] = 0;
        }
    }

    return dest - buffer;
}

size_t CSVWriter::escape(char* buffer, size_t bufferSize, StringView string)
{
    if (buffer && !PRIME_GUARD(bufferSize)) {
        buffer = NULL;
    }

    if (string.empty()) {
        if (buffer) {
            *buffer = 0;
        }
        return 0;
    }

    const char* const chars = string.begin();
    size_t const length = string.size();

    if (ASCIIIsSpaceOrTab(chars[0]) || ASCIIIsSpaceOrTab(chars[length - 1])) {
        return escapeInQuotes(buffer, bufferSize, string);
    }

    // Might only need to quote strings containing " and ' if they're the first/last characters?

    for (size_t i = 0; i != length; ++i) {
        if (ASCIIIsControlCharacter(chars[i]) || chars[i] == '"' || chars[i] == '\"' || chars[i] == ',') {
            return escapeInQuotes(buffer, bufferSize, string);
        }
    }

    StringCopy(buffer, bufferSize, chars);
    return length;
}

std::string CSVWriter::escape(StringView string)
{
    size_t size = escape(NULL, 0, string);

    std::string result(size, 0);

    escape(&result[0], size, string);

    return result;
}

std::string CSVWriter::escapeInQuotes(StringView string)
{
    size_t size = escapeInQuotes(NULL, 0, string);

    std::string result(size, 0);

    if (size) {
        escapeInQuotes(&result[0], size, string);
    }

    return result;
}

CSVWriter::CSVWriter()
{
    construct();
}

void CSVWriter::construct()
{
    _needComma = false;
    _error = false;
    _newline = "\r\n";
}

CSVWriter::CSVWriter(Stream* outputStream, Log* log, size_t bufferSize, void* buffer)
{
    construct();
    init(outputStream, log, bufferSize, buffer);
}

void CSVWriter::init(Stream* stream, Log* log, size_t bufferSize, void* buffer)
{
    _streamBuffer.init(stream, bufferSize, buffer);
    _log = log;
    beginWrite();
}

void CSVWriter::beginWrite()
{
    _needComma = false;
}

void CSVWriter::setNewline(StringView newline)
{
    _newline.assign(newline.begin(), newline.end());
}

bool CSVWriter::writeCell(StringView cell)
{
    if (_needComma && !writeRaw(StringView(",", 1))) {
        return false;
    }

    _needComma = true;

    size_t space = _streamBuffer.getSpace();

    if (!space) {
        if (!flush()) {
            return false;
        }

        space = _streamBuffer.getSpace();
        PRIME_ASSERT(space); // flush() should've freed up some bytes for us.
    }

    size_t size = escape((char*)_streamBuffer.getWritePointer(), space, cell);

    if (size < space) {
        _streamBuffer.advanceWritePointer(size);
        return !_error;
    }

    char stackBuffer[PRIME_BIG_STACK_BUFFER_SIZE];

    if (size < sizeof(stackBuffer)) {
        escape(stackBuffer, sizeof(stackBuffer), cell);
        return writeRaw(StringView(stackBuffer, size));
    }

    _cellBuffer.resize(size + 1);
    escape(&_cellBuffer[0], _cellBuffer.size() + 1, cell);
    return writeRaw(StringView(_cellBuffer.data(), size));
}

bool CSVWriter::writeRaw(StringView text)
{
    if (!_streamBuffer.writeBytes(text.begin(), text.size(), _log)) {
        setErrorFlag(true);
        return false;
    }

    return true;
}

bool CSVWriter::endRow()
{
    _needComma = false;
    if (!_streamBuffer.writeBytes(_newline.data(), _newline.size(), _log)) {
        setErrorFlag(true);
        return false;
    }

    return true;
}

bool CSVWriter::flush()
{
    if (!_streamBuffer.flush(_log)) {
        setErrorFlag(true);
        return false;
    }

    return true;
}

bool CSVWriter::printfCell(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    bool result = vprintfCell(format, argptr);
    va_end(argptr);

    return result;
}

bool CSVWriter::vprintfCell(const char* format, va_list argptr)
{
    FormatBufferVA<> formatted(format, argptr);

    return writeCell(StringView(formatted.c_str(), formatted.getLength()));
}

bool CSVWriter::writeRow(ArrayView<const StringView> cells)
{
    bool ok = true;
    for (size_t i = 0; i != cells.size(); ++i) {
        if (!writeCell(cells[i])) {
            ok = false;
            break;
        }
    }

    return ok && endRow();
}
}
