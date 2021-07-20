// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_XMLNODEWRITER_H
#define PRIME_XMLNODEWRITER_H

#include "XMLNode.h"
#include "XMLWriter.h"

namespace Prime {

/// Writes a hierarchy of XML nodes to a file.
class PRIME_PUBLIC XMLNodeWriter {
public:
    XMLNodeWriter();

    enum { defaultBufferSize = PRIME_FILE_BUFFER_SIZE };

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _xmlProcessingInstruction(true)
            , _encoding("UTF-8")
            , _assumeText(true)
            , _ignoreNameCase(false)
            , _html(false)
        {
        }

        Options& setXMLProcessingInstruction(bool value)
        {
            _xmlProcessingInstruction = value;
            return *this;
        }
        bool getXMLProcessingInstruction() const { return _xmlProcessingInstruction; }

        Options& setEncoding(std::string value)
        {
            _encoding.swap(value);
            return *this;
        }
        const std::string& getEncoding() const { return _encoding; }

        /// By default true, so all nodes are assumed to contain text and therefore cannot be indented etc. as
        /// that would insert extra whitespace. Works alongside the text node filters set by addTextNodeFilters().
        Options& setAssumeText(bool value)
        {
            _assumeText = value;
            return *this;
        }
        bool getAssumeText() const { return _assumeText; }

        // In HTML mode, self-closing tags are only used for tags which are supposed to be empty in HTML
        // (as returned by GetHTMLEmptyElements()).
        Options& setHTML(bool value)
        {
            _html = value;
            return *this;
        }
        bool getHTML() const { return _html; }

        Options& setIgnoreNameCase(bool value)
        {
            _ignoreNameCase = value;
            return *this;
        }
        bool getIgnoreNameCase() const { return _ignoreNameCase; }

    private:
        bool _xmlProcessingInstruction;
        std::string _encoding;
        bool _assumeText;
        bool _ignoreNameCase;
        bool _html;
    };

    /// Assign the Stream and Log to write to. If buffer π null, allocate a buffer of the specified size,
    /// otherwise use the supplied memory for the buffer.
    XMLNodeWriter(const Options& options, Stream* stream, Log* log, size_t bufferSize = defaultBufferSize,
        void* buffer = NULL);

    ~XMLNodeWriter();

    void init(const Options& options, Stream* stream, Log* log, size_t bufferSize = defaultBufferSize,
        void* buffer = NULL);

    Log* getLog() const { return _writer.getLog(); }

    /// Add a bunch of names (all in the same namespace) to the list of nodes to filter from the text/non-text
    /// lists. So if you call setAssumeText(false), these will be the text nodes, if you call
    /// setAssumeText(true) (the default) then these will be the non-text nodes.
    void addTextNodeFilters(const char* const* names, size_t count, const char* namespaceForAll);

    bool isTextNode(StringView name, StringView nspace) const;

    /// In HTML mode, returns true for all element names returned by GetHTMLEmptyElements() and false for all
    /// others. When not in HTML mode, returns true.
    bool allowSelfClosing(StringView name) const;

    bool writeDocument(const XMLNode* node, bool childrenOnly);

    XMLWriter& getWriter() { return _writer; }

private:
    static bool equalNamespaces(StringView a, StringView b);
    bool equalNames(StringView a, StringView b) const;

    bool write(const XMLNode* node);

    bool writeChildren(const XMLNode* node, bool includingAttributes);

    XMLWriter _writer;
    Options _options;

    struct NameAndNamespace {
        const char* name;
        const char* nspace;
    };

    std::vector<NameAndNamespace> _names;
};
}

#endif
