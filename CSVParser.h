// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CSVPARSER_H
#define PRIME_CSVPARSER_H

#include "TextReader.h"
#include <string>

namespace Prime {

/// Parse CSV files.
class PRIME_PUBLIC CSVParser {
public:
    enum Token {
        TokenError = -1,

        /// Returned at the end of the file.
        TokenEOF = 0,

        /// Returned at the end of each line.
        TokenNewline = 1,

        /// Returned for a normal text token.
        TokenText = 2,
    };

    CSVParser();

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _excelMode(false)
            , _delimiter(',')
        {
        }

        Options& setDelimiter(char value)
        {
            _delimiter = value;
            return *this;
        }
        char getDelimiter() { return _delimiter; }

        /// In Excel mode, a quote must appear immediately after the delimiter otherwise the value is considered
        /// unquoted.
        Options& setExcelMode(bool value = true)
        {
            _excelMode = value;
            return *this;
        }
        bool getExcelMode() const { return _excelMode; }

    private:
        bool _excelMode;
        char _delimiter;
    };

    void init(TextReader* textReader, const Options& options = Options())
    {
        _options = options;
        _textReader = textReader;
    }

    Token read();

    const std::string& getText() const { return _tokenText; }

    /// Return a Log that will prefix messages with the current location.
    Log* getLog() { return _textReader->getLog(); }

private:
    /// Report an error, prefixing the message with the current location.
    Token error(const char* format, ...);

    Token readError();

    Token parseText();

    Token parseQuotedText();

    Token skipNewline();

    bool skipSpacesAndTabs();

    RefPtr<TextReader> _textReader;

    Token _token;
    std::string _tokenText;

    Options _options;

    PRIME_UNCOPYABLE(CSVParser);
};
}

#endif
