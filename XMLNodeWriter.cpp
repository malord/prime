// Copyright 2000-2021 Mark H. P. Lord

#include "XMLNodeWriter.h"
#include "TextEncoding.h"

namespace Prime {

XMLNodeWriter::XMLNodeWriter()
{
}

XMLNodeWriter::XMLNodeWriter(const Options& options, Stream* stream, Log* log, size_t bufferSize, void* buffer)
{
    init(options, stream, log, bufferSize, buffer);
}

XMLNodeWriter::~XMLNodeWriter()
{
}

void XMLNodeWriter::init(const Options& options, Stream* stream, Log* log, size_t bufferSize, void* buffer)
{
    _options = options;
    _writer.init(
        XMLWriter::Options().setHTML(options.getHTML()),
        stream,
        log,
        bufferSize,
        buffer);
}

void XMLNodeWriter::addTextNodeFilters(const char* const* names, size_t count, const char* namespaceForAll)
{
    _names.reserve(_names.size() + count);
    for (size_t i = 0; i != count; ++i) {
        NameAndNamespace nn;
        nn.name = names[i];
        nn.nspace = namespaceForAll;
        _names.push_back(nn);
    }
}

bool XMLNodeWriter::equalNames(StringView a, StringView b) const
{
    return _options.getIgnoreNameCase() ? ASCIIEqualIgnoringCase(a, b) : StringsEqual(a, b);
}

bool XMLNodeWriter::equalNamespaces(StringView a, StringView b)
{
    return ASCIIEqualIgnoringCase(a, b);
}

bool XMLNodeWriter::allowSelfClosing(StringView name) const
{
    if (_options.getHTML()) {
        for (auto& tag : GetHTMLEmptyElements()) {
            if (StringsEqualIgnoringCase(tag, name)) {
                return true;
            }
        }

        return false;
    }

    return true;
}

bool XMLNodeWriter::isTextNode(StringView name, StringView nspace) const
{
    for (size_t i = 0; i != _names.size(); ++i) {
        const NameAndNamespace& nn = _names[i];
        if (equalNames(nn.name, name) && equalNamespaces(nn.nspace, nspace)) {
            return !_options.getAssumeText();
        }
    }

    return _options.getAssumeText();
}

bool XMLNodeWriter::writeDocument(const XMLNode* node, bool childrenOnly)
{
    if (_options.getXMLProcessingInstruction()) {
        _writer.writeProcessingInstruction("xml", Format("version=\"1.0\" encoding=\"%s\"", _options.getEncoding().c_str()));
        _writer.writeText("\n");
    }

    if (childrenOnly) {
        if (!writeChildren(node, false)) {
            return false;
        }
    } else {
        if (!write(node)) {
            return false;
        }
    }

    return _writer.end();
}

bool XMLNodeWriter::writeChildren(const XMLNode* node, bool includingAttributes)
{
    const XMLNode* firstNonAttribute = NULL;
    const XMLNode* child;

    // Write all the attributes first, in case the order has been mucked up.
    for (child = node->getFirstChild(); child; child = child->getNextSibling()) {
        if (child->isAttribute()) {
            if (!includingAttributes) {
                continue;
            }
            if (!write(child)) {
                return false;
            }
        } else {
            if (!firstNonAttribute) {
                firstNonAttribute = child;
            }
        }
    }

    // Now write everything else.
    for (child = firstNonAttribute; child; child = child->getNextSibling()) {
        if (child->isAttribute()) {
            continue; // already written all the attributes.
        }

        if (!write(child)) {
            return false;
        }
    }

    return true;
}

bool XMLNodeWriter::write(const XMLNode* node)
{
    switch (node->getType()) {
    case XMLNode::TypeElement:
        if (isTextNode(node->getName(), node->getNamespace())) {
            _writer.startTextElement(node->getQualifiedName());
        } else {
            _writer.startElement(node->getQualifiedName());
        }
        break;

    case XMLNode::TypeProcessingInstruction:
        if (!_options.getXMLProcessingInstruction() || !equalNames(node->getQualifiedName(), "xml")) {
            _writer.writeProcessingInstruction(node->getQualifiedName(), node->getValue());
        }
        break;

    case XMLNode::TypeAttribute:
        _writer.writeAttribute(node->getQualifiedName(), node->getValue());
        break;

    case XMLNode::TypeComment:
        _writer.writeComment(node->getValue());
        break;

    case XMLNode::TypeDocType:
        _writer.writeDOCTYPE(node->getValue());
        break;

    case XMLNode::TypeText:
        if (node->isCDATA()) {
            _writer.writeCDATA(node->getValue());
        } else if (node->isEncodedText()) {
            _writer.writeEscapedText(node->getValue());
        } else {
            _writer.writeText(node->getValue());
        }
        break;
    }

    if (!writeChildren(node, true)) {
        return false;
    }

    if (node->isElement()) {
        _writer.endElement(allowSelfClosing(node->getName()));
    }

    return !_writer.getErrorFlag();
}
}
