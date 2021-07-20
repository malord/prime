// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_XMLPULLPARSER_H
#define PRIME_XMLPULLPARSER_H

#include "ArrayView.h"
#include "ScopedPtr.h"
#include "TextEncoding.h"
#include "TextReader.h"
#include <map>
#include <set>
#include <string.h>

namespace Prime {

/// An XML pull parser. Only supports char encodings - use an IconvReader to support other encodings.
class PRIME_PUBLIC XMLPullParser : public NonAtomicRefCounted<XMLPullParser> {
public:
    enum Token {
        /// Returned when an error occurs.
        TokenError = -2,

        /// Returned when the end of the file is encountered. This is only returned if outside of any elements,
        /// otherwise TokenError is returned instead.
        TokenEOF = -1,

        /// Used internally - never returned.
        TokenNone = 0,

        /// Characters or whitespace have been read. Use getText() to access the text.
        TokenText = 1,

        /// A processing instruction has been read. Use getName() to get the processing instruction (should be
        /// "xml").
        TokenProcessingInstruction = 2,

        /// The start of an element has been read. Use getName() to get the element name.
        TokenStartElement = 3,

        /// The end of an element has been read. Use getName() to get the name of the element that has ended.
        /// XMLPullParser ensures end element tags match the correct start element tag.
        TokenEndElement = 5,

        /// A comment was read. Use getText() to get the comment text.
        TokenComment = 6,

        /// A doctype element was read. Use getText() to get the doctype text.
        TokenDocType = 7
    };

    enum ErrorCode {
        /// No error has occurred.
        ErrorNone = 0,

        /// A read error occurred.
        ErrorReadFailed = 1,

        /// Whitespace found at an unexpected point.
        ErrorUnexpectedWhitespace = 2,

        /// Unknown entity reference.
        ErrorUnknownEntity = 3,

        /// Invalid entity reference.
        ErrorInvalidEntity = 4,

        /// An illegal character was found.
        ErrorInvalidCharacter = 5,

        /// End of file was encountered unexpectedly.
        ErrorUnexpectedEndOfFile = 6,

        /// An invalid name was found.
        ErrorIllegalName = 7,

        /// Expected an = after an attribute name.
        ErrorExpectedEquals = 8,

        /// Expected an " around an attribute value.
        ErrorExpectedQuote = 9,

        /// Expected a > after an element or end element.
        ErrorExpectedRightAngleBracket = 10,

        /// An end element was encountered outside any elements.
        ErrorUnexpectedEndElement = 11,

        /// A mismatched end element was found.
        ErrorMismatchedEndElement = 12,

        /// Expected text but got something else.
        ErrorExpectedText = 13,

        /// Expected an empty element.
        ErrorExpectedEmptyElement = 14,

        /// Text was found outside any element. This probably means the file is not an XML file.
        ErrorTextOutsideElement = 15,

        /// Unknown namespace prefix.
        ErrorUnknownNamespace = 16,

        /// -- within a comment.
        ErrorIncorrectlyTerminatedComment = 17,

        /// Invalid character within attribute value.
        ErrorInvalidAttributeValue = 18,

        /// Found ]]> in normal text.
        ErrorCDATATerminatorInText = 19,

        /// <! not followed by DOCTYPE.
        ErrorInvalidDocType = 20,

        /// Same attribute name found multiple times.
        ErrorDuplicateAttribute = 21,

        ErrorMultipleTopLevelElements = 22,
    };

    /// Returns a pointer to a localised description of an ErrorCode.
    static const char* getErrorDescription(ErrorCode error);

    XMLPullParser() { construct(); }

    ~XMLPullParser();

    enum Conformance {
        /// Accept XML, and common, insevere errors.
        ConformanceDefault = 0,

        /// Accept strict XML, and fail on minor errors. This is require to pass XML conformance test.
        ConformanceStrict = 1,

        /// Accept XML and SGML and try to recover from all erros.
        ConformanceLenient = 2,
    };

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _conformance(ConformanceDefault)
            , _caseInsensitiveNames(false)
            , _htmlEntities(false)
            , _htmlMode(false)
        {
        }

        Options& setConformance(Conformance value)
        {
            _conformance = value;
            return *this;
        }
        Conformance getConformance() const { return _conformance; }

        Options& setCaseInsensitiveNames(bool value = true)
        {
            _caseInsensitiveNames = value;
            return *this;
        }
        bool getCaseInsensitiveNames() const { return _caseInsensitiveNames; }

        /// If enabled (it's not, by default) then HTML standard character entities (e.g., &nbsp;) are handled.
        Options& setHTMLEntities(bool value = true)
        {
            _htmlEntities = value;
            return *this;
        }
        bool getHTMLEntities() const { return _htmlEntities; }

        /// If enabled (it's not, by default) then HTML empty elements are correctly processed.
        Options& setHTMLMode(bool value = true)
        {
            _htmlMode = value;
            return *this;
        }
        bool getHTMLMode() const { return _htmlMode; }

        Options& setAllHTMLOptions()
        {
            setConformance(ConformanceLenient);
            setCaseInsensitiveNames(true);
            setHTMLEntities(true);
            setHTMLMode(true);
            return *this;
        }

    private:
        Conformance _conformance;
        bool _caseInsensitiveNames;
        bool _htmlEntities;
        bool _htmlMode;
    };

    void init(TextReader* textReader, const Options& options = Options());

    Log* getLog() const { return _textReader->getLog(); }

    const Options& getOptions() const { return _options; }

    bool isLenient() const { return _options.getConformance() == ConformanceLenient; }

    bool isStrict() const { return _options.getConformance() == ConformanceStrict; }

    /// Set a list of elements which are "empty", and do not require end tags.
    void addEmptyElements(ArrayView<const char*> elements, const char* namespaceForAll);

    typedef HTMLEntity Entity;

    /// These replace the built in set ("&amp;", "&lt;", etc.). The supplied array must remain valid for the life
    /// of this object. Since we don't support DTDs, this is the only way to get custom entities in to XMLPullParser.
    void setUserEntities(ArrayView<const Entity> entities);

    /// Returns a pointer to a description of a Token.
    const char* getTokenDescription(int token) const;

    /// Read the next token.
    int read();

    /// Like read(), but limited to returning TokenStartElement, TokenText, TokenEndElement and TokenEOF,
    /// thereby emulating a Java XMLPullParser. (Skips processing instructions, comments and doctypes.)
    int next();

    ErrorCode getError() const { return _error; }

    /// If the last token read was an element, skips everything until the end of that element. Returns false if
    /// an error occurs along the way.
    bool skipElement();

    /// Call this to read an empty element. If the element is found to not be empty, an error is logged and false
    /// is returned.
    bool skipEmptyElement();

    /// Read the contents of a text-only element. Call this after reading the start element of the element you
    /// want to read. elementDescription is used in error messages.
    const char* readWholeText(const char* elementDescription);

    /// As readWholeText() but trims leading and trailing whitespace.
    const char* readWholeTextTrimmed(const char* elementDescription);

    bool isCDATA() const { return _cdata; }

    /// Returns the local name (i.e., without namespace prefix) of the element.
    const char* getName() const { return _localName; }

    /// Returns the local name (i.e., without namespace prefix) of the element.
    const char* getLocalName() const { return _localName; }

    /// Returns the length of the element name that was read.
    size_t getNameLength() const { return strlen(_localName); }

    /// Returns the name with prefix (e.g., mynamespace:myelement).
    const char* getQualifiedName() const { return _qualifiedName; }

    /// Returns the namespace of the current name.
    const char* getNamespace() const { return _namespace; }

    /// Returns the text that was read.
    const std::string& getText() const { return _text; }

    /// Returns the length of the text that was read.
    size_t getTextLength() const { return _text.size(); }

    /// Returns true if the text is whitespace only.
    bool isTextEntirelyWhitespace() const;

    size_t getAttributeCount() const
    {
        PRIME_DEBUG_ASSERT(!_elements.empty());
        return _elements.back().attributes.size();
    }

    struct Attribute {
        const char* qualifiedName;
        const char* nspace;
        const char* localName;
        const char* value;
    };

    Attribute getAttribute(size_t index) const;

    /// Returns an empty Attribute (all members are "") if a matching attribute can't be found.
    Attribute getAttribute(StringView localName) const;

    /// Returns an empty Attribute (all members are "") if a matching attribute can't be found.
    Attribute getAttribute(StringView localName, StringView nspace) const;

    /// Returns -1 if a matching attribute can't be found.
    ptrdiff_t getAttributeIndex(StringView localName) const;

    /// Returns -1 if a matching attribute can't be found.
    ptrdiff_t getAttributeIndex(StringView localName, StringView nspace) const;

private:
    static const Attribute emptyAttribute;

    struct InternedAttribute {
        const char* qualifiedName; // An intern'd string.
        const char* nspace;
        const char* localName;
        size_t valueOffset;

        InternedAttribute()
            : qualifiedName(NULL)
            , nspace(NULL)
            , localName(NULL)
            , valueOffset(0)
        {
        }
    };

    struct Element {
        const char* name;
        bool isScript;

        std::vector<InternedAttribute> attributes;

        std::string values;
    };

    Attribute getAttributeCheckIndex(ptrdiff_t index) const;

    void construct();

    /// Set the value of error and log an error message. Returns TokenError.
    int setError(ErrorCode error);

    void warn(ErrorCode error);

    int read2();

    void readFailed() { setError(ErrorReadFailed); }

    /// Like setError(), but returns false.
    bool setErrorReturnFalse(ErrorCode code)
    {
        setError(code);
        return false;
    }

    bool setErrorReturnFalseUnlessLenient(ErrorCode code);

    struct StringLess {
        bool operator()(const char* a, const char* b) const
        {
            return strcmp(a, b) < 0;
        }
    };

    class StringTable {
    public:
        ~StringTable();

        /// Intern a string in to the string table.
        const char* intern(const char* string);

        /// Intern a substring in to the string table.
        const char* intern(const char* string, size_t len);

    private:
        // Element and attribute names are kept in a string table to reduce memory allocations.
        typedef std::set<char*, StringLess> StringsSet;
        StringsSet _strings;

        // Buffer used by intern()
        std::string _internBuffer;
    };

    /// Parse an element, first checking to see if it's a processing instruction, CDATA or DOCTYPE.
    int parseElement();

    void pushElement(const char* name);
    void popNamespaces();
    void popElement();
    void setTopElementNamespace();
    void setNamespace(const char* name, const char* value);
    void deleteNamespaces();

    int parseStartElement(bool* skipable);
    int parseEndElement(bool* skipable);
    int parseExclamation();
    int parseProcessingInstruction();
    int parseCDATA();
    int parseComment();
    int parseDocType();

    void processCR();
    void processLF();
    bool processCRLF(int c)
    {
        if (c == 10) {
            processLF();
            return true;
        } else if (c == 13) {
            processCR();
            return true;
        }

        return false;
    }

    bool isValidText(int c);

    bool processAmpersand();
    bool processCharacterNumber();
    bool processHexCharacterNumber();
    bool addUnicodeChar(uint32_t n);

    bool skipWhitespace();
    bool skipWhitespaceIfLenient();
    bool parseAttribute(bool* skipable);
    bool parseAttributeValue();
    bool parseUnquotedAttributeValue();

    /// Parse a name, storing it in name. On error, *skipable is set to true if the error can be ignored and the
    /// whole element should just be treated as malformed text.
    bool parseName(bool* skipable);

    /// Parse text.
    int parseText();

    /// In HTML mode, used for processing <script> and <style> tags.
    bool readScriptString();
    bool readScriptSingleLineComment();
    bool readScriptMultiLineComment();

    /// Set the current name, returned with getName(). Figures out the namespace, removing any namespace prefix.
    void setNameAndDetermineNamespace(const char* name);

    void determineNamespaceAndLocalName(const char* name, const char** localName, const char** nspace);

    /// Find the default namespace (xmlns=).
    const char* findNamespace();
    const char* findNamespaceOld(); // For testing.

    /// Find a prefixed namespace (xmlns:prefix=).
    const char* findNamespace(const char* prefix);
    const char* findNamespaceOld(const char* prefix); // For testing.

    bool isEmptyElement(const char* name, const char* nspace) const;
    bool canElementBeHere() const;
    int findAncestor(const char* localName) const;

    static bool equalNamespaces(const char* a, const char* b);

    bool equalNames(const char* a, const char* b) const;

    bool inScript() const { return !_elements.empty() && _elements.back().isScript; }

    //
    // Data
    //

    RefPtr<TextReader> _textReader;

    Options _options;

    ErrorCode _error;
    std::string _text;
    std::string _wholeText;

    struct Namespace {
        const char* name;
        const char* value;
        size_t depth;
        Namespace* prev;
    };

    typedef std::map<const char*, Namespace*, StringLess> NamespaceMap;
    NamespaceMap _namespaces;

    // Stack of elements we're inside.
    std::vector<Element> _elements;

    bool _hadFirstTopLevelElement;

    struct NameAndNamespace {
        const char* name;
        const char* nspace;
    };

    std::vector<NameAndNamespace> _emptyElements;

    ArrayView<const Entity> _entities;

    // Last name parsed with parseName().
    const char* _name;

    // Name to return to the caller.
    const char* _localName;

    // Qualified (prefixed) version of _localName.
    const char* _qualifiedName;

    // Namespace of the current name.
    const char* _namespace;

    // Value to return to the caller.
    const char* _currentValue;

    // Was the last thing processed an empty element?
    bool _emptyElement;

    // Number of elements to pop before the next read().
    int _popElement;

    // The last Token returned by read().
    int _lastToken;

    StringTable _stringTable;

    // Buffer used by parseName()
    std::string _parseNameBuffer;

    // Set to true if the TokenText that was just read was a CDATA.
    bool _cdata;

    PRIME_UNCOPYABLE(XMLPullParser);
};
}

#endif
