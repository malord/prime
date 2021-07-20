// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_XMLWRITER_H
#define PRIME_XMLWRITER_H

#include "Config.h"
#include "StreamBuffer.h"
#include <vector>

namespace Prime {

/// Write XML documents dealing with formatting and escaping.
class PRIME_PUBLIC XMLWriter {
public:
    class PRIME_PUBLIC Options {
    public:
        Options()
            : _html(false)
        {
        }

        Options& setHTML(bool value)
        {
            _html = value;
            return *this;
        }
        bool getHTML() const { return _html; }

    private:
        bool _html;
    };

    XMLWriter();

    /// Assign the Stream and Log to write to. If buffer is null, allocate a buffer of the specified size,
    /// otherwise use the supplied memory for the buffer.
    XMLWriter(const Options& options, Stream* stream, Log* log, size_t bufferSize, void* buffer = NULL);

    ~XMLWriter();

    void init(const Options& options, Stream* stream, Log* log, size_t bufferSize, void* buffer = NULL);

    Log* getLog() const { return _log; }

    /// Asserts that the root elements has been ended and flushes the Stream so you can call getError() to
    /// check for errors. Returns true on success, false if any errors have occurred.
    bool end();

    /// Flush the Stream without checking whether we've correctly written a complete XML file.
    bool flush();

    /// Reset for re-use, flushing the Stream and clearing the error flag. Returns false if the error flag was set
    /// (it may have been set by flushing the Stream).
    bool reset();

    /// Returns true if any errors have occurred since the stream was set.
    bool getErrorFlag() const { return _errors; }

    /// Write raw text directly to the output stream. The text is not escaped.
    void writeRaw(StringView text);

    /// Write a comment.
    void writeComment(StringView comment);

    /// Write a DOCTYPE.
    void writeDOCTYPE(StringView text);

    /// Start an element that will contain only other elements. The element name StringView must remain valid until
    /// the element is ended.
    void startElement(StringView name);

    /// Start an element that will contain text. The element name pointers must remain valid until the element is
    /// ended. In a text element, whitespace is significant so indenting is disabled.
    void startTextElement(StringView name);

    /// Add an attribute to the element that was just started. Attributes must be written before any text or
    /// comments are written inside an element.
    void writeAttribute(StringView name, StringView value);

    /// It's up to the application to write the XML processing instruction (<?xml version=...?>).
    void writeProcessingInstruction(StringView name, StringView content);

    /// Write text to the current element.
    void writeText(StringView text);

    /// Write text which has already been escaped.
    void writeEscapedText(StringView text);

    /// Write text as CDATA to the current element.
    void writeCDATA(StringView text);

    /// End the element last started with startElement() or startTextElement().
    void endElement(bool allowSelfClosing = true);

    /// Write a text-only element with no attributes.
    void writeTextElement(StringView name, StringView content)
    {
        startTextElement(name);
        writeText(content);
        endElement();
    }

private:
    struct Element {
        StringView name;
        bool isText;
        bool isOpen;
    };

    void beginWrite();

    void startElement(StringView name, bool isText);
    void writeIndent();
    void closeElement();
    void writeEscaped(StringView string);
    void writeCommentEscaped(StringView string);
    void writeCDATAEscaped(StringView string);
    void closeElementAndWriteIndent();

    void writeTextInternal(StringView string, bool escape);

    // If you don't nest more than 20 elements you won't incur memory allocation.
    std::vector<Element> _elements;

    StreamBuffer _streamBuffer;
    RefPtr<Log> _log;
    Options _options;
    bool _errors;
    int _currentIndent;

    PRIME_UNCOPYABLE(XMLWriter);
};
}

#endif
