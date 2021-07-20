// Copyright 2000-2021 Mark H. P. Lord

#include "CSVParser.h"
#include "PrefixLog.h"
#include "StringUtils.h"

namespace Prime {

CSVParser::CSVParser()
{
    _textReader = NULL;
    _token = TokenEOF;
}

CSVParser::Token CSVParser::error(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    getLog()->logVA(Log::LevelError, format, argptr);
    va_end(argptr);
    return TokenError;
}

CSVParser::Token CSVParser::readError()
{
    return error(PRIME_LOCALISE("Read error."));
}

CSVParser::Token CSVParser::read()
{
    // Once an error's found, keep returning error.
    if (_token == TokenError) {
        return TokenError;
    }

    int ch = _textReader->peekChar();

    _textReader->setTokenStartToCurrentPointer();

    if (ch == TextReader::ErrorChar) {
        return (_token = readError());
    }

    if (ch == TextReader::EOFChar) {
        return (_token = TokenEOF);
    }

    // parseText() always reads trailing whitespace, so there's not going to be whitespace followed by a newline.
    if (ASCIIIsNewline(ch)) {
        return (_token = skipNewline());
    }

    return (_token = parseText());
}

CSVParser::Token CSVParser::skipNewline()
{
    int ch = _textReader->readChar();
    int ch2 = _textReader->peekChar();

    if (ch == TextReader::ErrorChar || ch2 == TextReader::ErrorChar) {
        return readError();
    }

    if (ch == '\r') {
        if (ch2 == '\n') {
            // Windows \r\n
            _textReader->skipChar();
        }
    } else if (ch == '\n') {
        if (ch2 == '\r') {
            // Olde Mac \n\r
            _textReader->skipChar();
        }
    }

    return TokenNewline;
}

CSVParser::Token CSVParser::parseText()
{
    if (_options.getExcelMode()) {
        int ch = _textReader->peekChar();

        // Excel only considers a cell to be quoted if the first character
        // after the comma or newline is a double quote.
        if (ch == '"') {
            return parseQuotedText();
        }

        // Error/EOF will be caught the next time the TextReader is read.
    }

    // Skip leading spaces and tabs.
    if (!skipSpacesAndTabs()) {
        return TokenError;
    }

    int ch = _textReader->peekChar();

    if (ch == TextReader::ErrorChar) {
        return readError();
    }

    if (ch == TextReader::EOFChar) {
        return TokenEOF;
    }

    if (!_options.getExcelMode() && ch == '"') {
        // If not emulating Excel, allow leading space before the double quote.
        return parseQuotedText();
    }

    _tokenText.resize(0);

    const char delimiter = _options.getDelimiter();

    for (;;) {
        const char* begin = _textReader->getReadPointer();
        const char* top = _textReader->getTopPointer();

        if (begin == top) {
            ptrdiff_t got = _textReader->fetchMore();
            if (got < 0) {
                return readError();
            }
            if (got == 0) {
                break;
            }

            begin = _textReader->getReadPointer();
            top = _textReader->getTopPointer();
        }

        const char* ptr = begin;
        while (ptr != top && *ptr != delimiter && !ASCIIIsNewline(*ptr)) {
            ++ptr;
        }

        _tokenText.append(begin, ptr);
        _textReader->skipChars((size_t)(ptr - begin));

        if (ptr != top) {
            if (*ptr == delimiter) {
                _textReader->skipChar();
            }

            break;
        }
    }

    StringRightTrimInPlace(_tokenText);
    return TokenText;
}

CSVParser::Token CSVParser::parseQuotedText()
{
    int quote = _textReader->readChar();
    (void)quote;
    PRIME_ASSERT(quote == '"'); // you might choose to get crazy and also allow an apostrophe here. Even a backtick.

    _tokenText.resize(0);

    for (;;) {
        int ch = _textReader->readChar();

        if (ch == TextReader::ErrorChar) {
            return readError();
        }

        if (ch == TextReader::EOFChar) {
            error(PRIME_LOCALISE("Missing terminating double quote."));
            return TokenError;
        }

        if (ch == '"') {
            int ch2 = _textReader->peekChar();

            if (ch2 == TextReader::ErrorChar) {
                return readError();
            }

            if (ch2 != '"') {
                break;
            }

            // An escaped double quote.
            _textReader->skipChar();
        }

        _tokenText += TextReader::intToChar(ch);
    }

    if (!skipSpacesAndTabs()) {
        return TokenError;
    }

    int ch = _textReader->peekChar();

    if (ch == TextReader::ErrorChar) {
        return readError();
    }

    if (ch == TextReader::EOFChar || ASCIIIsNewline(ch)) {
        return TokenText;
    }

    if (ch == _options.getDelimiter()) {
        _textReader->skipChar();
        return TokenText;
    }

    error(PRIME_LOCALISE("Text after terminating double quote."));
    return TokenError;
}

bool CSVParser::skipSpacesAndTabs()
{
    for (;;) {
        // Usually only skip one character so I've not optimised this at all
        int ch = _textReader->peekChar();

        if (ch == TextReader::ErrorChar) {
            readError();
            return false;
        }

        if (!ASCIIIsSpaceOrTab(ch)) {
            return true;
        }

        _textReader->skipChar();
    }
}
}
