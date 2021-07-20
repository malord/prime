// Copyright 2000-2021 Mark H. P. Lord

#include "QuotedPrintableEncoder.h"

namespace Prime {

static const uint8_t hexDigit[] = "0123456789ABCDEF";

QuotedPrintableEncoder::QuotedPrintableEncoder()
{
    construct();
}

QuotedPrintableEncoder::QuotedPrintableEncoder(Stream* stream, const Options& options)
{
    construct();
    begin(stream, options);
}

QuotedPrintableEncoder::~QuotedPrintableEncoder()
{
    end(Log::getNullLog());
}

void QuotedPrintableEncoder::construct()
{
    _started = false;
    _maxLineLength = 0;
}

void QuotedPrintableEncoder::begin(Stream* stream, const Options& options)
{
    _stream = stream;
    _putBack = UINT_MAX;
    _options = options;

    PRIME_ASSERT(_options.getLineLength() >= 8);
    _maxLineLength = _options.getLineLength();
    _line.reset(new uint8_t[_maxLineLength + 3]);
    _lineLength = 0;

    _started = true;
}

ptrdiff_t QuotedPrintableEncoder::writeSome(const void* memory, size_t maximumBytes, Log* log)
{
    PRIME_ASSERT(_started);

    if (maximumBytes == 0) {
        return 0;
    }

    const uint8_t* ptr = (const uint8_t*)memory;
    const uint8_t* end = ptr + maximumBytes;

    ptrdiff_t wrote = 0;
    const TextMode textMode = _options.getTextMode();

    if (_putBack != UINT_MAX) {
        // We have a \r in search of a \n. Grab the next byte and push those two bytes.
        uint8_t buf[2];
        buf[0] = (uint8_t)_putBack;
        _putBack = UINT_MAX;
        buf[1] = ptr[0];
        wrote = writeSome(buf, 2, log);
        PRIME_ASSERT(wrote != 0); // We have two characters, at least one should've been written.
        if (wrote <= 0) {
            return -1;
        }
        // We already claimed to have written the putback byte on the last call.
        ptr += wrote - 1;
    }

    size_t badDotPosition = _options.getEscapeDot() ? 0 : (size_t)-1;
    size_t badDashPosition = _options.getEscapeDash() ? 0 : (size_t)-1;

    for (; ptr != end;) {
        uint8_t ch = *ptr;

        // Newlines in the input need to be written as \r\n in the output.
        if ((ch == '\r' && textMode != TextModeBinary) || (ch == '\n' && textMode == TextModeText)) {

            if (ptr + 1 == end) {
                _putBack = ch; // Remember this character so we can try for a two character sequence on the next run.
                ++ptr; // Consume it.
                break;
            }

            if (textMode == TextModeText || ptr[1] == '\n') {
                // Consume the input newline (1 or 2 bytes).
                if (ch == '\r') {
                    if (ptr[1] == '\n') {
                        //ch = '\n';
                        ++ptr;
                    }

                } else {
                    if (ptr[1] == '\r') {
                        //ch = '\r';
                        ++ptr;
                    }
                }

                if (!flushLine(log)) {
                    return -1;
                }

                ++ptr;
                continue;
            }

            // We have a \r in a binary file - it'll be escaped.
        }

        // We don't have a newline in the input, so if we've reached the line length then we need to wrap.
        if (_lineLength >= _maxLineLength) {
            PRIME_ASSERT(_lineLength == _maxLineLength);

            unsigned int wasLength;
            uint8_t was[3];

            if (_line[_lineLength - 3] == '=') {
                // Had an escaped sequence on the end of the line.
                wasLength = 3;
                was[0] = '=';
                was[1] = _line[_lineLength - 2];
                was[2] = _line[_lineLength - 1];
                _lineLength -= 2; // Leave the = on the end of the line as a soft line break

            } else {
                wasLength = 1;
                was[0] = _line[_lineLength - 1];
                _line[_lineLength - 1] = '='; // Replace with a soft line break
            }

            if (!flushLine(log)) {
                return -1;
            }

            PRIME_ASSERT(_lineLength + wasLength <= _maxLineLength);

            if (wasLength == 1) {
                // Write the last character again (it may need escaping now it's at column 0).
                ch = was[0];
                --ptr;
            } else {
                // Copy the escaped character to the next line.
                for (unsigned int i = 0; i != wasLength; ++i) {
                    _line[_lineLength++] = was[i];
                }
            }
        }

        // Write spaces and tabs unescaped and let flushLine() deal with any at the end of a line.
        if ((ch >= 32 && ch <= 127 && ch != '=' && (ch != '.' || _lineLength != badDotPosition) && (ch != '-' || _lineLength != badDashPosition)) || (ch == '\t')) {
            _line[_lineLength++] = ch;

        } else {
            // We have a character that needs escaping.
            if (_lineLength + 3 > _maxLineLength) {
                // No room for the escape sequence?
                _line[_lineLength++] = '=';
                if (!flushLine(log)) {
                    return -1;
                }
            }

            _line[_lineLength++] = '=';
            _line[_lineLength++] = hexDigit[(ch >> 4) & 0x0f];
            _line[_lineLength++] = hexDigit[ch & 0x0f];
        }

        ++ptr;
    }

    return ptr - (const uint8_t*)memory;
}

bool QuotedPrintableEncoder::flushLine(Log* log, bool crlf)
{
    PRIME_ASSERT(_lineLength <= _maxLineLength);

    uint8_t moved = 0;

    if (_lineLength) {
        uint8_t ch = _line[_lineLength - 1];

        if (ch == '\t' || ch == ' ') {
            // The line ends with a space or tab. See if we have room to convert it to an escape sequence.
            if (_lineLength + 2 <= _maxLineLength) {
                _line[_lineLength - 1] = '=';
                _line[_lineLength++] = hexDigit[(ch >> 4) & 0x0f];
                _line[_lineLength++] = hexDigit[ch & 0x0f];

            } else {
                // We'll move the character to the start of the next line.
                _line[_lineLength - 1] = '=';
                moved = ch;
            }
        }
    }

    if (crlf || moved) {
        _line[_lineLength++] = '\r';
        _line[_lineLength++] = '\n';
    }

    if (!_stream->writeExact(_line.get(), _lineLength, log)) {
        return false;
    }

    if (moved) {
        _line[0] = moved;
        _lineLength = 1;
        return flushLine(log, crlf);
    }

    _lineLength = 0;
    return true;
}

bool QuotedPrintableEncoder::end(Log* log)
{
    if (!_started) {
        return true;
    }

    bool ok = true;

    if (_putBack != UINT_MAX) {
        // TextModeBinary doesn't use putback
        PRIME_ASSERT(_options.getTextMode() != TextModeBinary);

        if (_options.getTextMode() == TextModeText) {
            // In text mode, if we've ended on a \r or \n then emit a newline.
            ok = flushLine(log, true);

        } else {
            // In binary mode, if we've ended on a \r then emit it escaped.
            PRIME_ASSERT(_putBack == '\r');

            if (_lineLength + 3 > _maxLineLength) {
                _line[_lineLength++] = '=';
                ok = flushLine(log, true);
            }

            _line[_lineLength++] = '=';
            _line[_lineLength++] = '0';
            _line[_lineLength++] = 'D';
            ok = flushLine(log, false) && ok;
        }

    } else if (_lineLength) {
        // We've ended on a line without a newline.
        ok = flushLine(log, false);

    } else {
        ok = true;
    }

    _started = false;
    return ok;
}

bool QuotedPrintableEncoder::close(Log* log)
{
    if (_started) {
        bool ok = end(log);
        _started = false;
        return _stream->close(log) && ok;
    }

    return true;
}

bool QuotedPrintableEncoder::flush(Log* log)
{
    if (_started) {
        return _stream->flush(log);
    }

    return true;
}
}
