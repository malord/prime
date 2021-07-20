// Copyright 2000-2021 Mark H. P. Lord

// TODO: pluggable Unicode support

#include "Lexer.h"
#include "NumberParsing.h"
#include "StringUtils.h"
#include "TextEncoding.h"

namespace Prime {

void Lexer::construct() PRIME_NOEXCEPT
{
    _textReader = NULL;
    _error = ErrorNone;
    _keywords = _keywordsEnd = NULL;
    _operators = _operatorsEnd = NULL;
    _wordChars = NULL;
    _fakeToken = TokenNone;
}

Lexer::~Lexer()
{
}

int Lexer::setError(int errorNumber)
{
    _error = errorNumber;

    getLog()->error(PRIME_LOCALISE("%s"), getErrorDescription(errorNumber));

    return TokenError;
}

bool Lexer::expect(int expectedToken, bool canSkipNewline)
{
    for (;;) {
        int got = read();
        if (got == TokenError) {
            return false;
        }

        if (got == TokenNewline) {
            if (canSkipNewline) {
                continue;
            }
        }

        if (got == expectedToken || (expectedToken == TokenReal && got == TokenInteger)) {
            return true;
        }

        getLog()->error(PRIME_LOCALISE("Expected: %s, got: %s"), getTokenDescription(expectedToken), getTokenDescription(got));
        return false;
    }
}

bool Lexer::expectInteger(intmax_t& number)
{
    int token = read();
    if (token == TokenError) {
        return false;
    }
    if (token != TokenInteger) {
        getLog()->error(PRIME_LOCALISE("Expected: integer, got: %s"), getTokenDescription(token));
        return false;
    }

    number = _integer;
    return true;
}

bool Lexer::expectReal(long double& number)
{
    int token = read();
    if (token == TokenError) {
        return false;
    }
    if (token != TokenInteger && token != TokenReal) {
        getLog()->error(PRIME_LOCALISE("Expected: number, got: %s"), getTokenDescription(token));
        return false;
    }

    number = _real;
    return true;
}

bool Lexer::isNextStatement()
{
    TextReader::Marker m(_textReader);
    return isNextStatement(read());
}

bool Lexer::isNextStatement(int token) const
{
    if (token == TokenError) {
        return false;
    }

    return token == Lexer::TokenNewline || token == Lexer::TokenEOF || (isOperator(token) && StringsEqual(getText(), ";"));
}

bool Lexer::expectNextStatement()
{
    int token = read();
    if (token == TokenError) {
        return false;
    }

    if (isNextStatement(token)) {
        return true;
    }

    getLog()->error(PRIME_LOCALISE("Expected: next statement, got: %s"), getTokenDescription(token));
    return false;
}

void Lexer::unexpected(int token)
{
    getLog()->error(PRIME_LOCALISE("Unexpected: %s"), getTokenDescription(token));
}

const char* Lexer::getTokenDescription(int token) const
{
    switch (token) {
    case TokenError:
        return PRIME_LOCALISE("error");
    case TokenEOF:
        return PRIME_LOCALISE("end of source");
    case TokenNone:
        return PRIME_LOCALISE("nothing");
    case TokenComment:
        return PRIME_LOCALISE("comment");
    case TokenWhitespace:
        return PRIME_LOCALISE("whitespace");
    case TokenNewline:
        return PRIME_LOCALISE("newline");
    case TokenString:
        return PRIME_LOCALISE("string");
    case TokenIdentifier:
        return PRIME_LOCALISE("identifier");
    case TokenInteger:
        return PRIME_LOCALISE("integer number");
    case TokenReal:
        return PRIME_LOCALISE("floating point number");
    case TokenOperator:
        return PRIME_LOCALISE("unknown symbol");
    }

    if (token >= TokenFirstKeyword && token < TokenFirstOperator) {
        token -= TokenFirstKeyword;
        if (token < (int)(_keywordsEnd - _keywords)) {
            return _keywords[token];
        }

        return PRIME_LOCALISE("invalid keyword");
    }

    if (token >= TokenFirstOperator) {
        token -= TokenFirstOperator;
        if (token < (int)(_operatorsEnd - _operators)) {
            return _operators[token];
        }

        return PRIME_LOCALISE("invalid operator");
    }

    return PRIME_LOCALISE("unknown token");
}

const char* Lexer::getErrorDescription(int errorNumber)
{
    switch (errorNumber) {
    case ErrorReadFailed:
        return PRIME_LOCALISE("Read error");
    case ErrorUnterminatedComment:
        return PRIME_LOCALISE("End of file within multi-line comment");
    case ErrorUnterminatedString:
        return PRIME_LOCALISE("String not terminated");
    case ErrorInvalidHexEscape:
        return PRIME_LOCALISE("Invalid hexadecimal escape");
    case ErrorUnknownEscape:
        return PRIME_LOCALISE("Unknown escape character");
    case ErrorExpectDigitAfterSign:
        return PRIME_LOCALISE("Expect digit after + or -");
    case ErrorInvalidNumber:
        return PRIME_LOCALISE("Invalid number");
    default:
        return PRIME_LOCALISE("Unknown error");
    }
}

void Lexer::skipNewline(int first)
{
    if (first == '\r') {
        if (_textReader->peekChar() == '\n') {
            _text += '\n';
            _textReader->skipChar();
        }
    } else {
        PRIME_ASSERT(first == '\n');
        if (_textReader->peekChar() == '\r') {
            _text += '\r';
            _textReader->skipChar();
        }
    }
}

bool Lexer::skipWhitespacePastNewline()
{
    bool foundNewline = false;

    for (;;) {
        int c = _textReader->peekChar();

        if (c < 0) {
            if (c == TextReader::EOFChar) {
                return true;
            }

            if (c == TextReader::ErrorChar) {
                setError(ErrorReadFailed);
                return false;
            }
        }

        if (isNewline(c)) {
            if (foundNewline) {
                return true;
            }

            foundNewline = true;
            _textReader->skipChar();
            _text += TextReader::intToChar(c);
            skipNewline(c);
            continue;
        }

        if (!isWhitespace(c)) {
            return true;
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }
}

void Lexer::putBack(int token) PRIME_NOEXCEPT
{
    PRIME_ASSERTMSG(_fakeToken == TokenNone, "Can only putBack a single token.");
    _fakeToken = token;
}

int Lexer::read()
{
    if (_fakeToken != TokenNone) {
        int token = _fakeToken;
        _fakeToken = TokenNone;
        return token;
    }

    int token;
    do {
        int c = _textReader->peekChar();

        _textReader->setTokenStartToCurrentPointer();

        _text.resize(0);

        if (isWhitespace(c)) {
            token = lexWhitespace();

        } else if (isWordStart(c) || isCustomWordChar(c)) {
            token = lexWord();

        } else if (isQuote(c)) {
            token = lexQuotedString();

        } else if (isDigit(c)) {
            token = lexNumber();

        } else if (_options.getSignedNumbers() && (c == '-' || c == '+')) {
            token = lexSignedNumber();

        } else if (c == '/') {
            int c2 = _textReader->peekChar(1);
            if (c2 == '/') {
                token = lexSingleLineComment();

            } else if (c2 == '*') {
                token = lexMultiLineComment();

            } else {
                token = lexOperator();
            }

        } else if (c == '#' && _options.getHashCommentsEnabled()) {
            token = lexSingleLineComment();

        } else if (c < 0) {
            if (c == TextReader::EOFChar) {
                return TokenEOF;
            }

            PRIME_ASSERT(c == TextReader::ErrorChar);
            return setError(ErrorReadFailed);

        } else if (c == '\\' && isWhitespace(_textReader->peekChar(1))) {
            token = lexWhitespace();

        } else {
            token = lexOperator();
        }
    } while (token == TokenNone);

    return token;
}

int Lexer::peek()
{
    Marker marker(this);
    return read();
}

int Lexer::lexNewline()
{
    int c = _textReader->readChar();

    PRIME_ASSERT(isNewline(c));

    _text += TextReader::intToChar(c);
    skipNewline(c);

    return TokenNewline;
}

int Lexer::lexWhitespace()
{
    for (;;) {
        int c = _textReader->peekChar();

        if (!isWhitespace(c)) {
            // this includes TextReader::EOFChar.
            if (c == TextReader::ErrorChar) {
                return setError(ErrorReadFailed);
            }

            if (c == '\\') {
                if (isWhitespace(_textReader->peekChar(1))) {
                    _text += '\\';
                    _textReader->skipChar();

                    if (!skipWhitespacePastNewline()) {
                        return TokenError;
                    }

                    continue;
                }
            }

            break;
        }

        // Newlines need to be specially handled for option.getWantNewlines.
        if (_options.getWantNewlines() && isNewline(c)) {
            if (_text.empty()) {
                return lexNewline();
            }

            break;
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    return (_options.getWantWhitespace()) ? TokenWhitespace : TokenNone;
}

int Lexer::lexSingleLineComment()
{
    for (;;) {
        int c = _textReader->readChar();

        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                return setError(ErrorReadFailed);
            }

            if (c == TextReader::EOFChar) {
                break;
            }
        }

        if (isNewline(c)) {
            _text += TextReader::intToChar(c);
            skipNewline(c);
            break;
        }

        if (c == '\\') {
            _text += TextReader::intToChar(c);

            // Always skip one character.
            c = _textReader->readChar();
            if (c == TextReader::ErrorChar) {
                return setError(ErrorReadFailed);
            }

            _text += TextReader::intToChar(c);

            if (!skipWhitespacePastNewline()) {
                return TokenError;
            }
        } else {
            _text += TextReader::intToChar(c);
        }
    }

    return (_options.getWantComments()) ? TokenComment : TokenNone;
}

int Lexer::lexMultiLineComment()
{
    // Skip the two characters we already know about.
    _text += '/';
    _text += '*';
    _textReader->skipChars(2);

    for (;;) {
        int c = _textReader->readChar();

        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                return setError(ErrorReadFailed);
            }

            if (c == TextReader::EOFChar) {
                return setError(ErrorUnterminatedComment);
            }
        }

        if (c == '\\') {
            _text += TextReader::intToChar(c);

            // Always skip one character.
            c = _textReader->readChar();
            if (c == TextReader::ErrorChar) {
                return setError(ErrorReadFailed);
            }

            _text += TextReader::intToChar(c);

            if (!skipWhitespacePastNewline()) {
                return TokenError;
            }

            continue;
        }

        _text += TextReader::intToChar(c);

        if (c == '*') {
            if (_textReader->peekChar() == '/') {
                _text += '/';
                _textReader->skipChar();
                break;
            }
        }
    }

    return (_options.getWantComments()) ? TokenComment : TokenNone;
}

bool Lexer::lexHexEscape(char escape)
{
    int digits = (escape == 'x') ? 2 : 4;
    bool started = false;
    uint32_t n = 0;

    while (digits--) {
        int c = _textReader->peekChar();

        if (c >= '0' && c <= '9') {
            n = n * 16 + (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            n = n * 16 + (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            n = n * 16 + (c - 'A' + 10);
        } else {
            if (!started) {
                setError(ErrorInvalidHexEscape);
                return false;
            }

            break;
        }

        if (_options.getVerbatim()) {
            _text += TextReader::intToChar(c);
        }

        started = true;
        _textReader->skipChar();
    }

    if (!_options.getVerbatim()) {
        if (escape == 'x') {
            _text += TextReader::intToChar((int)n);
        } else {
            PRIME_ASSERT(escape == 'u');
            uint8_t buffer[8];
            size_t len = UTF8Encode(buffer, n);

            _text.append((char*)buffer, len);
        }
    }

    return true;
}

bool Lexer::lexOctEscape()
{
    int digits = 3;
    unsigned long n = 0;

    while (digits--) {
        int c = _textReader->peekChar();

        if (c >= '0' && c <= '7') {
            n = n * 8 + (c - '0');
        } else {
            break;
        }

        if (_options.getVerbatim()) {
            _text += TextReader::intToChar(c);
        }

        _textReader->skipChar();
    }

    if (!_options.getVerbatim()) {
        _text += TextReader::intToChar((int)n);
    }
    return true;
}

bool Lexer::lexEscape()
{
    int c = _textReader->readChar();
    if (c == TextReader::ErrorChar) {
        setError(ErrorReadFailed);
        return false;
    }

    int n;

    switch (c) {
    case 'x':
    case 'X':
        if (_options.getVerbatim()) {
            _text += TextReader::intToChar(c);
        }

        return lexHexEscape('x');

    case 'u':
    case 'U':
        if (_options.getVerbatim()) {
            _text += TextReader::intToChar(c);
        }

        return lexHexEscape('u');

    case 'a':
        n = '\a';
        break;

    case 'b':
        n = '\b';
        break;

    case 'f':
        n = '\f';
        break;

    case 'n':
        n = '\n';
        break;

    case 'r':
        n = '\r';
        break;

    case 't':
        n = '\t';
        break;

    case 'v':
        n = '\v';
        break;

    case '"':
    case '\\':
    case '\'':
        n = c;
        break;

    case '0':
        if (_options.getVerbatim()) {
            _text += TextReader::intToChar(c);
        }

        return lexOctEscape();

    default:
        if (_options.getAllowUnknownEscapes()) {
            n = c;
            break;
        }

        setError(ErrorUnknownEscape);
        return false;
    }

    if (_options.getVerbatim()) {
        _text += TextReader::intToChar(c);
    } else {
        _text += TextReader::intToChar(n);
    }

    return true;
}

int Lexer::lexQuotedString()
{
    _quote = _textReader->readChar();
    if (_options.getVerbatim()) {
        _text += TextReader::intToChar(_quote);
    }

    for (;;) {
        int c = _textReader->readChar();

        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                return setError(ErrorReadFailed);
            }

            if (c == TextReader::EOFChar) {
                return setError(ErrorUnterminatedString);
            }
        }

        if (isNewline(c)) {
            if (!_options.getAllowNewlineInString()) {
                return setError(ErrorUnterminatedString);
            }
        }

        if (c == '\\' && !_options.getNoEscapes()) {
            if (_options.getSimpleEscapes()) {
                if (_options.getVerbatim()) {
                    _text += TextReader::intToChar(c);
                }

                int escaped = _textReader->readChar();
                if (escaped == TextReader::ErrorChar) {
                    return setError(ErrorReadFailed);
                }

                _text += TextReader::intToChar(escaped);
            } else if (isWhitespace(_textReader->peekChar())) {
                if (_options.getVerbatim()) {
                    _text += TextReader::intToChar(c);
                }

                if (!skipWhitespacePastNewline()) {
                    return TokenError;
                }
            } else {
                if (_options.getVerbatim()) {
                    _text += '\\';
                }

                if (!lexEscape()) {
                    return TokenError;
                }
            }

            continue;
        }

        if (c == _quote) {
            if (_options.getVerbatim()) {
                _text += TextReader::intToChar(c);
            }
            break;
        }

        _text += TextReader::intToChar(c);
    }

    return TokenString;
}

int Lexer::lexWord()
{
    for (;;) {
        int c = _textReader->readChar();

        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                return setError(ErrorReadFailed);
            }

            if (c == TextReader::EOFChar) {
                break;
            }
        }

        if (!isWord(c) && !isCustomWordChar(c)) {
            _textReader->putBack();
            break;
        }

        _text += TextReader::intToChar(c);
    }

    for (const char* const* k = _keywords; k != _keywordsEnd; ++k) {
        if (StringsEqual(*k, _text)) {
            return TokenFirstKeyword + (int)(k - _keywords);
        }
    }

    return TokenIdentifier;
}

int Lexer::lexSignedNumber()
{
    _text += TextReader::intToChar(_textReader->readChar());
    return lexNumber();
}

int Lexer::lexHex()
{
    for (;;) {
        int c = _textReader->peekChar();

        if (!isHexDigit(c)) {
            break;
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    return finishLexInteger(16);
}

int Lexer::lexOct()
{
    for (;;) {
        int c = _textReader->peekChar();

        if (!isOctDigit(c)) {
            if (c == '8' || c == '9') {
                return setError(ErrorInvalidNumber);
            }

            break;
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    return finishLexInteger(8);
}

int Lexer::finishLexInteger(int base)
{
    if (!StringToInt(_text, _integer, base)) {
        if (_options.getConvertInvalidNumbersToStrings()) {
            return TokenString;
        } else {
            return setError(ErrorInvalidNumber);
        }
    }

    _real = (long double)_integer;

    // TODO: check for suffix (e.g., 'L', 'LL', 'U')

    return TokenInteger;
}

int Lexer::lexNumber()
{
    if (_textReader->peekChar() == '0') {
        int c2 = _textReader->peekChar(1);

        if (c2 == 'x' || c2 == 'X') {
            _text += '0';
            _text += TextReader::intToChar(c2);
            _textReader->skipChars(2);
            return lexHex();
        }

        if (isDigit(c2)) {
            _text += '0';
            _textReader->skipChars(1);
            return lexOct();
        }
    }

    bool foundDot = false;
    bool foundE = false;

    for (;;) {
        int c = _textReader->peekChar();

        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                return setError(ErrorReadFailed);
            }
            if (c == TextReader::EOFChar) {
                break;
            }
        }

        if (!isDigit(c)) {
            if (c == '.') {
                if (foundDot || foundE) {
                    break;
                }

                foundDot = true;
            } else if (c == 'e' || c == 'E') {
                if (foundE) {
                    break;
                }

                foundE = true;

                _text += TextReader::intToChar(c);
                _textReader->skipChar();

                c = _textReader->peekChar();
                if (c == '+' || c == '-') {
                    _text += TextReader::intToChar(c);
                    _textReader->skipChar();
                }

                continue;
            } else {
                break;
            }
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    if (!foundDot && !foundE) {
        return finishLexInteger(10);
    }

    char* err;
    _real = PRIME_STRTOFLOATMAX(_text.c_str(), &err);

    if (*err) {
        if (_options.getConvertInvalidNumbersToStrings()) {
            return TokenString;
        } else {
            return setError(ErrorInvalidNumber);
        }
    }

    // TODO: check for suffix (e.g., 'L', 'LL', 'U')

    return TokenReal;
}

int Lexer::lexOperator()
{
    unsigned int longestMatch = 0;
    const char* const* longestOp = NULL;

    for (const char* const* o = _operators; o != _operatorsEnd; ++o) {
        const char* op = *o;
        const char* p = _textReader->getReadPointer();
        for (;; ++op, ++p) {
            if (!*op) {
                unsigned int length = (unsigned int)(p - _textReader->getReadPointer());
                if (length > longestMatch) {
                    longestMatch = length;
                    longestOp = o;
                }

                break;
            }

            if (p == _textReader->getTopPointer()) {
                unsigned int length = (unsigned int)(p - _textReader->getReadPointer());
                int c = _textReader->peekChar(length);
                if (c < 0) {
                    if (c == TextReader::ErrorChar) {
                        return setError(ErrorReadFailed);
                    }
                    if (c == TextReader::EOFChar) {
                        break;
                    }
                }

                PRIME_ASSERT(_textReader->getReadPointer() != _textReader->getTopPointer());
                p = _textReader->getReadPointer() + length;
            }

            if (*op != *p) {
                break;
            }
        }
    }

    if (!longestOp) {
        int c = _textReader->readChar();
        if (c == TextReader::ErrorChar) {
            return setError(ErrorReadFailed);
        }

        _text += TextReader::intToChar(c);
        return TokenOperator;
    }

    _text.append(_textReader->getReadPointer(), longestMatch);
    _textReader->skipChars(longestMatch);

    return TokenFirstOperator + (int)(longestOp - _operators);
}
}
