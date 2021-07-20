// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_XMLNODEREADER_H
#define PRIME_XMLNODEREADER_H

#include "RefCounting.h"
#include "XMLNode.h"
#include "XMLPullParser.h"

namespace Prime {

/// Reads an XML document in to a tree of XMLNodes. Note that reading from an XMLPullParser (possibly via
/// XMLExpat) will always be more efficient.
class PRIME_PUBLIC XMLNodeReader {
public:
    /// Reads from XMLPullParser and adds children to node.
    static bool read(XMLPullParser* xmlParser, XMLNode* node);

    XMLNodeReader();

    enum { defaultBufferSize = PRIME_FILE_BUFFER_SIZE };

    bool readDocument(const XMLPullParser::Options& xmlParserOptions, Stream* stream, Log* log,
        size_t bufferSize = defaultBufferSize);

    bool readDocument(const XMLPullParser::Options& xmlParserOptions, TextReader* textReader);

    bool readDocument(XMLPullParser* xmlParser);

    /// Will return NULL unless readDocument() has been called.
    RefPtr<const XMLNode> getDocument() const { return _document; }

    /// Will return NULL unless readDocument() has been called.
    RefPtr<XMLNode> getDocument() { return _document; }

private:
    RefPtr<XMLNode> _document;
};
}

#endif
