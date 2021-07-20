// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_JSONREADER_H
#define PRIME_JSONREADER_H

#include "Lexer.h"
#include "Value.h"

namespace Prime {

/// Loads a JSON files in to a Value.
class PRIME_PUBLIC JSONReader {
public:
    enum { defaultBufferSize = PRIME_FILE_BUFFER_SIZE };

    static Value parse(StringView string, Log* log);

    explicit JSONReader();

    ~JSONReader();

    /// In order to support encodings other than UTF-8 you must supply an IconReader initialised with
    /// guessEncoding() (note that PropertyListReader does this). Returns an invalid Value on error.
    Value read(Stream* stream, Log* log, size_t bufferSize = defaultBufferSize);

    /// In order to support encodings other than UTF-8, the TextReader must be reading from an IconReader
    /// initialised with guessEncoding() (note that PropertyListReader does this). Returns an invalid Value on
    /// error.
    Value read(TextReader* textReader);

private:
    bool read(Value& out);
    bool readArray(Value& out);
    bool readDictionary(Value& out);

    Lexer* _lexer;

    PRIME_UNCOPYABLE(JSONReader);
};
}

#endif
