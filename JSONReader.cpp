// Copyright 2000-2021 Mark H. P. Lord

#include "JSONReader.h"

// FUTURE: Consider creating a JSONPullParser to allow custom JSON readers to be implemented that bypass Value

namespace Prime {

namespace {

    const char* const keywords[] = {
        "undefined", "null", "true", "false"
    };

    enum {
        TokenUndefined = Lexer::TokenFirstKeyword,
        TokenNull,
        TokenTrue,
        TokenFalse,
    };

    const char* const operators[] = {
        "{", "}", "[", "]", ",", ":"
    };

    enum {
        TokenLCurly = Lexer::TokenFirstOperator,
        TokenRCurly,
        TokenLSquare,
        TokenRSquare,
        TokenComma,
        TokenColon
    };
}

Value JSONReader::parse(StringView string, Log* log)
{
    TextReader textReader;
    textReader.setLog(log);
    textReader.setText(string);

    return JSONReader().read(&textReader);
}

JSONReader::JSONReader()
{
}

JSONReader::~JSONReader()
{
}

Value JSONReader::read(Stream* stream, Log* log, size_t bufferSize)
{
    TextReader textReader;
    textReader.setLog(log);
    textReader.setStream(stream, bufferSize);

    return read(&textReader);
}

Value JSONReader::read(TextReader* textReader)
{
    // Be permissive.
    Lexer::Options lexerOptions;
    lexerOptions.setSignedNumbers().setAllowNewlineInString().setAllowUnknownEscapes().setHashCommentsEnabled();

    Lexer lexer;
    lexer.init(textReader, lexerOptions);

    lexer.setKeywords(ArrayView<const char* const>(keywords, COUNTOF(keywords)));
    lexer.setOperators(ArrayView<const char* const>(operators, COUNTOF(operators)));

    _lexer = &lexer;

    Value got;
    if (!read(got)) {
        got = undefined;
    } else {
        if (lexer.read() != Lexer::TokenEOF) {
            textReader->getLog()->warning(PRIME_LOCALISE("JSON file contains additional content which has been ignored."));
        }
    }

    return got;
}

bool JSONReader::read(Value& out)
{
    int token = _lexer->read();

    switch (token) {
    case Lexer::TokenError:
        return false;

    case Lexer::TokenEOF:
        _lexer->getLog()->error(PRIME_LOCALISE("Unexpected end of JSON file."));
        return false;

    case TokenUndefined:
        out = undefined;
        return true;

    case TokenNull:
        out = null;
        return true;

    case TokenTrue:
        out = true;
        return true;

    case TokenFalse:
        out = false;
        return true;

    case Lexer::TokenString:
        out = Value(_lexer->getText());
        return true;

    case Lexer::TokenIdentifier:
        // This is an extension to the JSON spec. Maybe add a strict mode options?
        out = Value(_lexer->getText());
        return true;

    case Lexer::TokenReal:
        out = Value((Value::Real)_lexer->getReal());
        return true;

    case Lexer::TokenInteger:
        out = Value((Value::Integer)_lexer->getInteger());
        return true;

    case TokenLSquare:
        return readArray(out);

    case TokenLCurly:
        return readDictionary(out);

    default:
        break;
    }

    _lexer->unexpected(token);
    return false;
}

bool JSONReader::readArray(Value& out)
{
    Value::Vector array;

    for (;;) {
        Lexer::Marker marker(_lexer);

        int token = _lexer->read();

        if (token == TokenRSquare) {
            marker.release();
            break;
        }

        marker.rewind();

        array.push_back(undefined);

        if (!read(array.back())) {
            return false;
        }

        token = _lexer->read();

        if (token == TokenComma) {
            continue;
        }

        if (token == TokenRSquare) {
            break;
        }

        if (token == Lexer::TokenError) {
            return false;
        }

        _lexer->unexpected(token);
        return false;
    }

    out.accessVector().swap(array);
    return true;
}

bool JSONReader::readDictionary(Value& out)
{
    Value::Dictionary dictionary;

    for (;;) {
        Lexer::Marker marker(_lexer);

        int token = _lexer->read();

        if (token == TokenRCurly) {
            marker.release();
            break;
        }

        marker.rewind();

        Value key;
        if (!read(key)) {
            return false;
        }
        if (!key.isString()) {
            key = key.toString();
            //_lexer->getLog()->warning(PRIME_LOCALISE("key converted to string: %s"), key.c_str());
        }

        token = _lexer->read();

        if (token == Lexer::TokenError) {
            return false;
        }

        if (token != TokenColon) {
            _lexer->unexpected(token);
            return false;
        }

        Value& value = dictionary.access(PRIME_MOVE(key.accessString()));

        if (!read(value)) {
            return false;
        }

        token = _lexer->read();

        if (token == TokenComma) {
            continue;
        }

        if (token == TokenRCurly) {
            break;
        }

        if (token == Lexer::TokenError) {
            return false;
        }

        _lexer->unexpected(token);
        return false;
    }

    out.accessDictionary().swap(dictionary);
    return true;
}
}
