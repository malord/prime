// Copyright 2000-2021 Mark H. P. Lord

#include "XMLNodeReader.h"

namespace Prime {

XMLNodeReader::XMLNodeReader()
{
}

bool XMLNodeReader::readDocument(const XMLPullParser::Options& xmlParserOptions, Stream* stream, Log* log, size_t bufferSize)
{
    TextReader textReader;
    textReader.setLog(log);
    textReader.setStream(stream, bufferSize);

    return readDocument(xmlParserOptions, &textReader);
}

bool XMLNodeReader::readDocument(const XMLPullParser::Options& xmlParserOptions, TextReader* textReader)
{
    XMLPullParser xmlParser;
    xmlParser.init(textReader, xmlParserOptions);

    return readDocument(&xmlParser);
}

bool XMLNodeReader::readDocument(XMLPullParser* xmlParser)
{
    _document.reset(new XMLNode);
    return read(xmlParser, _document.get());
}

bool XMLNodeReader::read(XMLPullParser* xmlParser, XMLNode* node)
{
    XMLNode* fallBackNode = NULL;

    for (;;) {
        int token = xmlParser->read();

        if (token == XMLPullParser::TokenError) {
            return false;
        }

        if (token == XMLPullParser::TokenEOF) {
            break;
        }

        if (!node) {
            if (fallBackNode && xmlParser->isLenient()) {
                node = fallBackNode;
            } else {
                return false;
            }
        }

        if (token == XMLPullParser::TokenStartElement) {
            node = node->addChild(XMLNode::TypeElement);
            node->setQualifiedName(xmlParser->getQualifiedName(), xmlParser->getNamespace());

            for (size_t i = 0; i != xmlParser->getAttributeCount(); ++i) {
                XMLPullParser::Attribute attr = xmlParser->getAttribute(i);
                XMLNode* attrNode = node->addChild(XMLNode::TypeAttribute, attr.value);
                attrNode->setQualifiedName(attr.qualifiedName, attr.nspace);
            }
            continue;
        }

        if (token == XMLPullParser::TokenEndElement) {
            // If we're at the root (node will become null) then the next token should be TokenEOF. If it's not,
            // and we're in lenient mode, we'll fall back to the root node and add any children there, in case of
            // multiple root elements.
            fallBackNode = node;

            node = node->getParent();

            continue;
        }

        if (token == XMLPullParser::TokenProcessingInstruction) {
            XMLNode* child = node->addChild(XMLNode::TypeProcessingInstruction, xmlParser->getText());
            child->setQualifiedName(xmlParser->getQualifiedName(), xmlParser->getNamespace());
            continue;
        }

        XMLNode::Type type = XMLNode::TypeComment;

        switch (token) {
        case XMLPullParser::TokenComment:
            type = XMLNode::TypeComment;
            break;

        case XMLPullParser::TokenDocType:
            type = XMLNode::TypeDocType;
            break;

        case XMLPullParser::TokenText:
            type = XMLNode::TypeText;
            break;

        default:
            PRIME_ASSERT(0);
            return false;
        }

        XMLNode* child = node->addChild(type, xmlParser->getText());

        if (type == XMLNode::TypeText && xmlParser->isCDATA()) {
            child->setCDATA(true);
        }
    }

    return true;
}

}
