// Copyright 2000-2021 Mark H. P. Lord

#include "XMLWriter.h"
#include <string.h>

namespace {

// TODO: might be worth finding a common place for this? Or making them static methods so other modules can use them?

inline bool IsXMLWhitespace(char ch)
{
    return ch == 0x09 || ch == 0x0a || ch == 0x0d || ch == 0x0c || ch == 0x20;
}

inline const char* SkipXMLWhitespace(const char* from, const char* to)
{
    for (; from != to; ++from) {
        if (!IsXMLWhitespace(*from)) {
            return from;
        }
    }

    return from;
}

}

namespace Prime {

XMLWriter::XMLWriter()
{
}

XMLWriter::XMLWriter(const Options& options, Stream* stream, Log* log, size_t bufferSize, void* buffer)
{
    init(options, stream, log, bufferSize, buffer);
}

XMLWriter::~XMLWriter()
{
}

void XMLWriter::init(const Options& options, Stream* stream, Log* log, size_t bufferSize, void* buffer)
{
    reset();
    _streamBuffer.init(stream, bufferSize, buffer);
    _log = log;
    _options = options;
    beginWrite();
}

void XMLWriter::beginWrite()
{
    _currentIndent = 0;
    _errors = false;
}

bool XMLWriter::end()
{
    PRIME_ASSERT(_elements.empty()); // Didn't end all elements.

    return flush();
}

bool XMLWriter::flush()
{
    if (!_streamBuffer.flush(_log)) {
        _errors = true;
    }

    return !getErrorFlag();
}

bool XMLWriter::reset()
{
    bool success = flush();
    _errors = false;
    return success;
}

void XMLWriter::writeRaw(StringView string)
{
    if (!_streamBuffer.writeString(string, _log)) {
        _errors = true;
    }
}

void XMLWriter::writeDOCTYPE(StringView text)
{
    _errors = !_streamBuffer.writeBytes("<!", 2, _log) || _errors;
    //writeEscaped(text);
    _errors = !_streamBuffer.writeString(text, _log) || _errors;
    _errors = !_streamBuffer.writeByte('>', _log) || _errors;
}

void XMLWriter::writeComment(StringView string)
{
    // Should probably add a check for --> (actually, -- is illegal in a comment) in the comment. I'll leave that to the caller for now.
    closeElementAndWriteIndent();
    _errors = !_streamBuffer.writeBytes("<!-- ", 5, _log) || _errors;
    writeCommentEscaped(string);
    _errors = !_streamBuffer.writeBytes(" -->", 4, _log) || _errors;
}

void XMLWriter::writeCommentEscaped(StringView stringView)
{
    const char* string = stringView.begin();
    const char* ptr = stringView.begin();
    const char* end = stringView.end();
    const char* escape = NULL;
    for (;;) {
        if (ptr == end) {

        } else if (*ptr == '-' && end - ptr >= 3 && memcmp(ptr, "-->", 3) == 0) {
            escape = "-- >";
        } else {
            ++ptr;
            continue;
        }

        _errors = !_streamBuffer.writeBytes(string, ptr - string, _log) || _errors;

        if (ptr == end) {
            break;
        }

        _errors = !_streamBuffer.writeString(escape, _log) || _errors;
        escape = 0;

        ++ptr;
        string = ptr;
    }
}

void XMLWriter::closeElementAndWriteIndent()
{
    closeElement();

    if (!_elements.empty() && !_elements.back().isText) {
        _errors = !_streamBuffer.writeByte('\n', _log) || _errors;
        writeIndent();
    }
}

void XMLWriter::startElement(StringView name)
{
    startElement(name, false);
}

void XMLWriter::startTextElement(StringView name)
{
    startElement(name, true);
}

void XMLWriter::writeProcessingInstruction(StringView name, StringView content)
{
    closeElementAndWriteIndent();

    _errors = !_streamBuffer.writeBytes("<?", 2, _log) || _errors;

    _errors = !_streamBuffer.writeString(name, _log) || _errors;

    _errors = !_streamBuffer.writeBytes(" ", 1, _log) || _errors;

    _errors = !_streamBuffer.writeString(content, _log) || _errors;

    _errors = !_streamBuffer.writeBytes("?>", 2, _log) || _errors;
}

void XMLWriter::startElement(StringView name, bool isText)
{
    closeElementAndWriteIndent();

    _errors = !_streamBuffer.writeByte('<', _log) || _errors;

    _errors = !_streamBuffer.writeString(name, _log) || _errors;

    Element newElement;
    newElement.name = name;
    newElement.isText = isText;
    newElement.isOpen = true;
    ++_currentIndent;

    _elements.push_back(newElement);
}

void XMLWriter::closeElement()
{
    if (!_elements.empty() && _elements.back().isOpen) {
        _errors = !_streamBuffer.writeByte('>', _log) || _errors;
        _elements.back().isOpen = false;
    }
}

void XMLWriter::writeIndent()
{
    if (!_elements.empty() && !_elements.back().isText) {
        for (int i = 0; i != _currentIndent; ++i) {
            _errors = !_streamBuffer.writeByte('\t', _log) || _errors;
        }
    }
}

void XMLWriter::writeAttribute(StringView name, StringView value)
{
    PRIME_ASSERT(!_elements.empty()); // Need an element to have an attribute.
    PRIME_ASSERT(_elements.back().isOpen); // The > has been written.

    _errors = !_streamBuffer.writeByte(' ', _log) || _errors;
    _errors = !_streamBuffer.writeString(name, _log) || _errors;
    _errors = !_streamBuffer.writeBytes("=\"", 2, _log) || _errors;
    writeEscaped(value);
    _errors = !_streamBuffer.writeByte('"', _log) || _errors;
}

void XMLWriter::writeEscaped(StringView stringView)
{
    const char* string = stringView.begin();
    const char* ptr = string;
    const char* end = stringView.end();
    const char* escape = NULL;
    for (;;) {
        if (ptr != end) {
            if (*ptr == '<') {
                escape = "&lt;";
            } else if (*ptr == '>') {
                escape = "&gt;";
            } else if (*ptr == '\'') {
                escape = _options.getHTML() ? "&#39;" : "&apos;";
            } else if (*ptr == '"') {
                escape = "&quot;";
            } else if (*ptr == '&') {
                escape = "&amp;";
            } else {
                ++ptr;
                continue;
            }
        }

        _errors = !_streamBuffer.writeBytes(string, ptr - string, _log) || _errors;

        if (ptr == end) {
            break;
        }

        _errors = !_streamBuffer.writeString(escape, _log) || _errors;
        escape = 0;

        ++ptr;
        string = ptr;
    }
}

void XMLWriter::writeTextInternal(StringView string, bool escape)
{
    const char* begin = string.begin();
    const char* end = string.end();

    if (!_elements.empty()) {
        if (!_elements.back().isText) {
            begin = SkipXMLWhitespace(begin, end);

            if (begin != end) {
                _elements.back().isText = true;
            }
        }
    }

    if (begin == end) {
        return;
    }

    closeElement();

    if (escape) {
        writeEscaped(StringView(begin, end));
    } else {
        writeRaw(StringView(begin, end));
    }
}

void XMLWriter::writeText(StringView string)
{
    writeTextInternal(string, true);
}

void XMLWriter::writeEscapedText(StringView string)
{
    writeTextInternal(string, false);
}

void XMLWriter::writeCDATA(StringView text)
{
    if (!_elements.empty()) {
        _elements.back().isText = true;
    }

    closeElement();

    _errors = !_streamBuffer.writeBytes("<![CDATA[", 9, _log) || _errors;
    writeCDATAEscaped(text);
    _errors = !_streamBuffer.writeBytes("]]>", 3, _log) || _errors;
}

void XMLWriter::writeCDATAEscaped(StringView stringView)
{
    const char* string = stringView.begin();
    const char* ptr = string;
    const char* end = stringView.end();
    const char* escape = NULL;
    for (;;) {
        if (ptr == end) {

        } else if (*ptr == ']' && end - ptr >= 3 && memcmp(ptr, "]]>", 3) == 0) {
            escape = "]]><![CDATA[";
        } else {
            ++ptr;
            continue;
        }

        _errors = !_streamBuffer.writeBytes(string, ptr - string, _log) || _errors;

        if (ptr == end) {
            break;
        }

        _errors = !_streamBuffer.writeString(escape, _log) || _errors;
        escape = 0;

        ++ptr;
        string = ptr;
    }
}

void XMLWriter::endElement(bool allowSelfClosing)
{
    PRIME_ASSERT(!_elements.empty()); // More end elements than start elements.

    --_currentIndent;

    if (_elements.back().isOpen && allowSelfClosing) {
        _errors = !_streamBuffer.writeBytes("/>", 2, _log) || _errors;

    } else {
        if (_elements.back().isOpen) {
            _errors = !_streamBuffer.writeBytes(">", 1, _log) || _errors;
        }

        if (!_elements.back().isText) {
            _errors = !_streamBuffer.writeByte('\n', _log) || _errors;
            writeIndent();
        }

        _errors = !_streamBuffer.writeBytes("</", 2, _log) || _errors;
        _errors = !_streamBuffer.writeString(_elements.back().name, _log) || _errors;
        _errors = !_streamBuffer.writeByte('>', _log) || _errors;
    }

    _elements.pop_back();

    //if (_elements.empty())
    //  _errors = ! _streamBuffer.writeByte('\n', _log) || _errors;
}
}
