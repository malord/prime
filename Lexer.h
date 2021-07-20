// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_LEXER_H
#define PRIME_LEXER_H

#include "ArrayView.h"
#include "ScopedPtr.h"
#include "TextReader.h"
#include <string.h>

namespace Prime {

/// A lexical analyser for C-like languages. Splits a text file in to a stream of tokens (operators, keywords,
/// numbers, strings, etc.). (No preprocessor support.)
class PRIME_PUBLIC Lexer : public RefCounted {
public:
    /// Built-in tokens.
    enum Token {
        /// Returned when an error occurs.
        TokenError = -2,

        /// Returned at the end of the file.
        TokenEOF = -1,

        /// Special "nothing was read" token.
        TokenNone = 0,

        /// A comment was read. Comments are only read if options.wantComments has been set.
        TokenComment = 1,

        /// Whitespace was read. Whitespace is only read if options.wantWhitespace has been set.
        TokenWhitespace = 2,

        /// A newline token was read. Newlines are only read as tokens if options.wantNewlines has been set.
        TokenNewline = 3,

        /// A quoted string was read.
        TokenString = 4,

        /// An identifier (i.e., a non-keyword word).
        TokenIdentifier = 5,

        /// An integer was read.
        TokenInteger = 6,

        /// A floating point number was read.
        TokenReal = 7,

        /// A symbol that doesn't correspond to a known operator.
        TokenOperator = 8,

        /// Custom keywords start with this value.
        TokenFirstKeyword = 10000,

        /// Custom operators start with this value.
        TokenFirstOperator = 20000,
    };

    /// Lexer error codes.
    enum ErrorCode {
        /// No error has occurred.
        ErrorNone = 0,

        /// A read error occurred.
        ErrorReadFailed = 1,

        /// Missing the end of a multi-line comment.
        ErrorUnterminatedComment = 2,

        /// The terminating quote of a quoted string was not found before the end of the line/file. Newlines can
        /// be allowed in quoted strings by enabling options.allowNewlineInString.
        ErrorUnterminatedString = 3,

        /// A \\x or \\u escape was found without a following hex digit.
        ErrorInvalidHexEscape = 4,

        /// An unknown escape (e.g., \\k) was found. This error can be disabled with options.allowUnknownEscapes.
        ErrorUnknownEscape = 5,

        /// Occurs when options.signedNumbers is set but a + or - was found that wasn't followed by a digit.
        ErrorExpectDigitAfterSign = 6,

        /// A malformed number (too many digits, incorrect format) was found.
        ErrorInvalidNumber = 7,
    };

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _wantComments(false)
            , _wantWhitespace(false)
            , _allowNewlineInString(false)
            , _allowUnknownEscapes(false)
            , _simpleEscapes(false)
            , _noEscapes(false)
            , _verbatim(false)
            , _signedNumbers(false)
            , _wantNewlines(false)
            , _hashComments(false)
            , _convertInvalidNumbersToStrings(true)
            , _allowHyphensInIdentifiers(false)
        {
        }

        /// If enabled, the Lexer will return comments.
        Options& setWantComments(bool value = true)
        {
            _wantComments = value;
            return *this;
        }
        bool getWantComments() const { return _wantComments; }

        /// If enabled, the lexer will return whitespace.
        Options& setWantWhitespace(bool value = true)
        {
            _wantWhitespace = value;
            return *this;
        }
        bool getWantWhitespace() const { return _wantWhitespace; }

        /// Newline characters within a string should be permitted.
        Options& setAllowNewlineInString(bool value = true)
        {
            _allowNewlineInString = value;
            return *this;
        }
        bool getAllowNewlineInString() const { return _allowNewlineInString; }

        /// Allow unknown escape sequences (e.g., \\z). Simply emit the escaped character.
        Options& setAllowUnknownEscapes(bool value = true)
        {
            _allowUnknownEscapes = value;
            return *this;
        }
        bool getAllowUnknownEscapes() const { return _allowUnknownEscapes; }

        /// Use simple escapes. \\n will emit an 'n' rather than \x0a.
        Options& setSimpleEscapes(bool value = true)
        {
            _simpleEscapes = value;
            return *this;
        }
        bool getSimpleEscapes() const { return _simpleEscapes; }

        /// Do not allow escapes in character strings. A \\ found within a string will be passed through as the
        /// token's text.
        Options& setNoEscapes(bool value = true)
        {
            _noEscapes = value;
            return *this;
        }
        bool getNoEscapes() const { return _noEscapes; }

        /// Pass through the text of a token verbatim.
        Options& setVerbatim(bool value = true)
        {
            _verbatim = value;
            return *this;
        }
        bool getVerbatim() const { return _verbatim; }

        /// Allow signed numbers. A '+' or '-' will be assumed to be the start of a number.
        Options& setSignedNumbers(bool value = true)
        {
            _signedNumbers = value;
            return *this;
        }
        bool getSignedNumbers() const { return _signedNumbers; }

        /// If enabled, every newline returns a TokenNewline.
        Options& setWantNewlines(bool value = true)
        {
            _wantNewlines = value;
            return *this;
        }
        bool getWantNewlines() const { return _wantNewlines; }

        /// The # character is treated as a comment.
        Options& setHashCommentsEnabled(bool value = true)
        {
            _hashComments = value;
            return *this;
        }
        bool getHashCommentsEnabled() const { return _hashComments; }

        /// If enabled (it is by default), invalid numbers become strings.
        Options& setConvertInvalidNumbersToStrings(bool value = true)
        {
            _convertInvalidNumbersToStrings = value;
            return *this;
        }
        bool getConvertInvalidNumbersToStrings() const { return _convertInvalidNumbersToStrings; }

        /// If enabled (not by default), hyphens are permitted in identifier names (but not as the first character).
        Options& setAllowHyphensInIdentifiers(bool value = true)
        {
            _allowHyphensInIdentifiers = value;
            return *this;
        }
        bool getAllowHyphensInIdentifiers() const { return _allowHyphensInIdentifiers; }

    private:
        bool _wantComments;
        bool _wantWhitespace;
        bool _allowNewlineInString;
        bool _allowUnknownEscapes;
        bool _simpleEscapes;
        bool _noEscapes;
        bool _verbatim;
        bool _signedNumbers;
        bool _wantNewlines;
        bool _hashComments;
        bool _convertInvalidNumbersToStrings;
        bool _allowHyphensInIdentifiers;
    };

    static bool isOperator(int token) PRIME_NOEXCEPT
    {
        return token == TokenOperator || token >= TokenFirstOperator;
    }

    Lexer() PRIME_NOEXCEPT { construct(); }

    ~Lexer();

    void init(TextReader* textReader, const Options& options) PRIME_NOEXCEPT
    {
        _textReader = textReader;
        _options = options;
    }

    /// Returns a Log that prefixes messages with the current location.
    Log* getLog() const PRIME_NOEXCEPT { return _textReader->getLog(); }

    const Options& getOptions() const PRIME_NOEXCEPT { return _options; }

    Options& getOptions() PRIME_NOEXCEPT { return _options; }

    /// Set a keyword list. This is a null-pointer terminated array of pointers to C strings. A keyword is given
    /// the token TokenFirstkeyword plus its index in this array.
    void setKeywords(ArrayView<const char* const> keywords)
    {
        _keywords = keywords.begin();
        _keywordsEnd = keywords.end();
    }

    /// Set the operator list. This is a null-pointer terminated array of pointers to C strings. An operator is
    /// given the token TokenFirstoperator plus its index in this array.
    void setOperators(ArrayView<const char* const> operators)
    {
        _operators = operators.begin();
        _operatorsEnd = operators.end();
    }

    /// Set a string of characters which will be treated as valid characters within an identifier, in addition to
    /// A-Z, a-z, 0-9 and _.
    void setCustomWordChars(const char* wordChars) { _wordChars = wordChars; }

    /// Put a fake token to be read by the next read().
    void putBack(int token) PRIME_NOEXCEPT;

    /// Read the next token from the file.
    int read();

    /// Read the next token from the file then put it back.
    int peek();

    /// You can read directly from the TextReader and read() will resume where you leave the TextReader.
    TextReader* textReader() { return _textReader; }

    const char* getLocation() const PRIME_NOEXCEPT { return _textReader->getLocation(); }

    unsigned int getLine() const PRIME_NOEXCEPT { return _textReader->getLine(); }

    unsigned int getColumn() const PRIME_NOEXCEPT { return _textReader->getColumn(); }

    /// Returns the text of the token that was just parsed.
    const std::string& getText() const PRIME_NOEXCEPT { return _text; }

    /// Set the text. This is useful in conjunction with putBack().
    void setText(std::string text) { _text.swap(text); }

    /// If Lex just returned TokenString, this returns the quotation mark that encloses the string (e.g., '"',
    /// '\'' or '`').
    int getQuote() const PRIME_NOEXCEPT { return _quote; }

    /// If Lex just returned TokenInteger, returns the integer that was parsed.
    intmax_t getInteger() const PRIME_NOEXCEPT { return _integer; }

    /// If Lex just returned TokenInteger or TokenReal, returns the number that was parsed.
    long double getReal() const PRIME_NOEXCEPT { return _real; }

    /// If Lex just returned TokenError, this returns the error.
    int getError() const PRIME_NOEXCEPT { return _error; }

    /// Lex the next token and make sure it is what we expect. If not, emits an error message.
    bool expect(int expectedToken, bool canSkipNewline = false);

    /// Lex the next token. If it's not an integer, emit a localised error message and return false. Otherwise,
    /// set *number and return true.
    bool expectInteger(intmax_t& number);

    /// Lex the next token. If it's not a real, emit a localised error message and return false. Otherwise, set
    /// *number and return true.
    bool expectReal(long double& number);

    /// Expect either a newline or a ";" and, if that's not what we get, emit a localised error message and return
    /// false.
    bool expectNextStatement();

    /// Returns true if the next token is either a newline or ";". Does not read the token.
    bool isNextStatement();

    /// Emit an error message about an unexpected token.
    void unexpected(int token);

    /// Returns a localised description of a token number.
    const char* getTokenDescription(int token) const;

    /// Returns a localised description of an error message.
    static const char* getErrorDescription(int errorNumber);

    /// Mark a point in the file so that it can be rewound to.
    class PRIME_PUBLIC Marker {
    public:
        Marker(Lexer* lexer) PRIME_NOEXCEPT : _marker(lexer->_textReader),
                                              _lexer(lexer),
                                              _fakeToken(lexer->_fakeToken)
        {
        }

        ~Marker()
        {
            if (_marker.isLocked()) {
                rewind();
            }
        }

        /// Release the marker. This prevents it from rewinding when destructed.
        void release() PRIME_NOEXCEPT { _marker.release(); }

        /// Rewind to this marker.
        void rewind() PRIME_NOEXCEPT
        {
            _marker.rewind();
            _lexer->_fakeToken = _fakeToken;
        }

    private:
        TextReader::Marker _marker;
        Lexer* _lexer;
        int _fakeToken;
    };

private:
    friend class Marker;

    /// Set the value of _error and invoke the error handler. Returns TokenError.
    int setError(int errorNumber);

    /// Returns true if the character c is a newline.
    static bool isNewline(int c) PRIME_NOEXCEPT { return c == '\n' || c == '\r'; }

    /// Returns true if the character c is whitespace.
    static bool isWhitespace(int c) PRIME_NOEXCEPT { return c > 0 && c <= ' '; }

    /// Returns true if c is a letter.
    static bool isAlpha(int c) PRIME_NOEXCEPT { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

    /// Returns true if c is a digit.
    static bool isDigit(int c) PRIME_NOEXCEPT { return c >= '0' && c <= '9'; }

    /// Returns true if c is a hexadecimal digit.
    static bool isHexDigit(int c) PRIME_NOEXCEPT { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

    /// Returns true if c is an octal digit.
    static bool isOctDigit(int c) PRIME_NOEXCEPT { return c >= '0' && c <= '7'; }

    /// Returns true if the character c is valid as the start of a word.
    static bool isWordStart(int c) PRIME_NOEXCEPT { return isAlpha(c) || c == '_'; }

    /// Returns true if the character c is valid as the continuation of a word.
    static bool isWord(int c) PRIME_NOEXCEPT { return isAlpha(c) || c == '_' || isDigit(c); }

    /// Returns true if the character c is a quotation mark.
    static bool isQuote(int c) PRIME_NOEXCEPT { return c == '\'' || c == '"' || c == '`'; }

    /// Skip the second part of a newline. If \\r is supplied, the method will skip a \\n. If \\n is supplied, the
    /// method will skip a \\r. In either case the skipped character will be appended to the token.
    void skipNewline(int first);

    /// Skip all whitespace up to a second newline or a non-whitespace character. The skipped characters are
    /// appended to the token. Returns false if a read error occurred.
    bool skipWhitespacePastNewline();

    int lexNewline();
    int lexWhitespace();
    int lexSingleLineComment();
    int lexMultiLineComment();
    int lexQuotedString();
    int lexWord();
    int lexNumber();
    int lexHex();
    int lexOct();
    int lexSignedNumber();
    int lexOperator();

    /// After a \\ has been read from the file, process the escape sequence. Returns false on error.
    bool lexEscape();

    /// Parse the digits after a \\x or \\u.
    bool lexHexEscape(char escape);

    /// Parse the digits after a \\0.
    bool lexOctEscape();

    /// Code common to lexHex() and lexOct().
    int finishLexInteger(int base);

    /// Common constructor logic.
    void construct() PRIME_NOEXCEPT;

    /// Returns true if the specified character is a custom word character.
    bool isCustomWordChar(int c) const PRIME_NOEXCEPT
    {
        return (_wordChars && c > 0 && strchr(_wordChars, TextReader::intToChar(c)) != NULL) || (_options.getAllowHyphensInIdentifiers() && c == '-');
    }

    bool isNextStatement(int token) const;

    RefPtr<TextReader> _textReader;

    int _error;

    Options _options;
    const char* const* _keywords;
    const char* const* _keywordsEnd;
    const char* const* _operators;
    const char* const* _operatorsEnd;
    const char* _wordChars;

    std::string _text;
    int _quote;
    intmax_t _integer;
    long double _real;

    int _fakeToken;

    PRIME_UNCOPYABLE(Lexer);
};
}

#endif
