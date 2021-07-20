// Copyright 2000-2021 Mark H. P. Lord

// TODO: have more information in error messages
// TODO: have some way of plugging in proper Unicode support for identifiers, whitespace, etc.

#include "XMLPullParser.h"
#include "NumberUtils.h"
#include "ScopedPtr.h"
#include "StringUtils.h"
#include "TextEncoding.h"
#include <memory>

// Leaving this in until I'm sure of the new code.
#define TEST_NAMESPACE_MAP 1

namespace Prime {

namespace {

    const char cdataSectionHeader[] = "<![CDATA[";
    const char doctypeHeader[] = "<!DOCTYPE";

    inline bool IsXMLWhitespace(int c, bool lenient)
    {
        // TODO: UNICODE support
        return (c <= 32 && (c == 10 || c == 13 || c == 32 || c == 9 || (lenient && (c == 12))));
    }

#if 0
        inline bool IsExtendedNameStartChar(uint32_t uch)
        {
            if (uch >= 0xc0 && uch <= 0xd6) return true;
            if (uch >= 0xd8 && uch <= 0xf6) return true;
            if (uch >= 0xf8 && uch <= 0x2ff) return true;
            if (uch >= 0x370 && uch <= 0x37d) return true;
            if (uch >= 0x37f && uch <= 0x1fff) return true;
            if (uch >= 0x200c && uch <= 0x200d) return true;
            if (uch >= 0x2070 && uch <= 0x218f) return true;
            if (uch >= 0x2c00 && uch <= 0x2fef) return true;
            if (uch >= 0x3001 && uch <= 0xd7ff) return true;
            if (uch >= 0xf900 && uch <= 0xfdcf) return true;
            if (uch >= 0xfdf0 && uch <= 0xfffd) return true;
            if (uch >= 0x10000 && uch <= 0xeffff) return true;
            return false;
        }

        inline bool IsExtendedNameChar(uint32_t uch)
        {
            if (IsExtendedNameStartChar(uch)) return true;
            if (uch == 0xb7) return true;
            if (uch >= 0x0300 && uch <= 0x036f) return true;
            if (uch >= 0x203f && uch <= 0x2040) return true;
            return false;
        }
#endif

    /// Returns true if the Unicode character is valid as the start of a name.
    inline bool IsNameStartChar(int c, bool lenient)
    {
        if (lenient) {
            return !IsXMLWhitespace(c, true) && !strchr("/>=", c);
        } else {
            // Testing for c > 127 covers all the Unicode cases in UTF-8, but it's not strict enough.
            return (c > 127) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == ':';
        }
    }

    /// Returns true if the Unicode character is valid within a name.
    inline bool IsNameChar(int c, bool lenient)
    {
        if (lenient) {
            return IsNameStartChar(c, true);
        } else {
            // TODO: Unicode lookup
            return IsNameStartChar(c, false) || (c >= '0' && c <= '9') || c == '.' || c == '-';
        }
    }

    inline bool IsNameChar(int c, bool lenient, bool first)
    {
        return first ? IsNameStartChar(c, lenient) : IsNameChar(c, lenient);
    }

    inline bool IsXMLWhitespace(StringView string, bool lenient)
    {
        for (const char* ptr = string.begin(); ptr != string.end(); ++ptr) {
            if (!IsXMLWhitespace(*ptr, lenient)) {
                return false;
            }
        }

        return true;
    }

    inline size_t CountLeadingWhitespace(const char* begin, size_t length, bool lenient)
    {
        const char* end = begin + length;
        const char* ptr = begin;
        while (ptr != end && IsXMLWhitespace(*ptr, lenient)) {
            ++ptr;
        }

        return ptr - begin;
    }

    inline size_t CountTrailingWhitespace(const char* begin, size_t length, bool lenient)
    {
        const char* end = begin + length;
        const char* ptr = end;
        while (ptr != begin && IsXMLWhitespace(ptr[-1], lenient)) {
            --ptr;
        }

        return end - ptr;
    }

    inline bool IsXMLUnquotedAttributeValueChar(int c, bool lenient)
    {
        if (lenient) {
            // HTML attributes are anything but whitespace and any of "\"'`=<>".
            return !IsXMLWhitespace(c, true) && c != '>';
        } else {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || strchr("-._:", c) != NULL;
        }
    }

    const XMLPullParser::Entity xmlEntities[] = {
        // Note these are sorted.
        { "&amp;", '&', NULL },
        { "&apos;", '\'', NULL },
        { "&gt;", '>', NULL },
        { "&lt;", '<', NULL },
        { "&quot;", '"', NULL },
    };

    //
    // HTML
    //

    // There's also the issue of implicit start elements, e.g., a <tr> implies a <table>
}

const XMLPullParser::Attribute XMLPullParser::emptyAttribute = { "", "", "", "" };

const char* XMLPullParser::getErrorDescription(ErrorCode error)
{
    switch (error) {
    case ErrorReadFailed:
        return PRIME_LOCALISE("Read error");
    case ErrorNone:
        return PRIME_LOCALISE("Unknown error");
    case ErrorUnexpectedWhitespace:
        return PRIME_LOCALISE("Unexpected whitespace");
    case ErrorUnknownEntity:
        return PRIME_LOCALISE("Unknown entity reference");
    case ErrorInvalidEntity:
        return PRIME_LOCALISE("Invalid entity reference");
    case ErrorInvalidCharacter:
        return PRIME_LOCALISE("Invalid character");
    case ErrorUnexpectedEndOfFile:
        return PRIME_LOCALISE("Unexpected end of file");
    case ErrorIllegalName:
        return PRIME_LOCALISE("Invalid name");
    case ErrorExpectedEquals:
        return PRIME_LOCALISE("Expected = after attribute name");
    case ErrorExpectedQuote:
        return PRIME_LOCALISE("Expected \" enclosing attribute value");
    case ErrorExpectedRightAngleBracket:
        return PRIME_LOCALISE("Expected >");
    case ErrorUnexpectedEndElement:
        return PRIME_LOCALISE("Unexpected end element");
    case ErrorMismatchedEndElement:
        return PRIME_LOCALISE("Mismatched end element");
    case ErrorExpectedText:
        return PRIME_LOCALISE("Expected text but got an element or attribute");
    case ErrorExpectedEmptyElement:
        return PRIME_LOCALISE("Expected an empty element");
    case ErrorTextOutsideElement:
        return PRIME_LOCALISE("Text outside element");
    case ErrorUnknownNamespace:
        return PRIME_LOCALISE("Unknown namespace");
    case ErrorIncorrectlyTerminatedComment:
        return PRIME_LOCALISE("Incorrectly terminated comment");
    case ErrorInvalidAttributeValue:
        return PRIME_LOCALISE("Invalid character in attribute value");
    case ErrorCDATATerminatorInText:
        return PRIME_LOCALISE("]]> found in text");
    case ErrorInvalidDocType:
        return PRIME_LOCALISE("Invalid DOCTYPE");
    case ErrorDuplicateAttribute:
        return PRIME_LOCALISE("Attribute name occurs more than once");
    case ErrorMultipleTopLevelElements:
        return PRIME_LOCALISE("Multiple top-level elements");
    }

    return "unknown XML error";
}

const char* XMLPullParser::getTokenDescription(int token) const
{
    switch (token) {
    case TokenError:
        return PRIME_LOCALISE("error");
    case TokenEOF:
        return PRIME_LOCALISE("end of file");
    case TokenNone:
        return PRIME_LOCALISE("null");
    case TokenText:
        return PRIME_LOCALISE("text");
    case TokenProcessingInstruction:
        return PRIME_LOCALISE("processing instruction");
    case TokenStartElement:
        return PRIME_LOCALISE("start element");
    case TokenEndElement:
        return PRIME_LOCALISE("end element");
    case TokenComment:
        return PRIME_LOCALISE("comment");
    case TokenDocType:
        return PRIME_LOCALISE("doctype");
    }

    return "unknown XML token";
}

void XMLPullParser::construct()
{
    _textReader = NULL;
    _options = Options();
    _text.resize(0);
    _text.reserve(2048);
    _wholeText.reserve(2048);
    _error = ErrorNone;
    _emptyElement = false;
    _popElement = 0;
    _name = NULL;
    _localName = NULL;
    _qualifiedName = NULL;
    _namespace = NULL;
    _entities = ArrayView<const Entity>(xmlEntities, COUNTOF(xmlEntities));
    _hadFirstTopLevelElement = false;
    _lastToken = TokenNone;
}

XMLPullParser::~XMLPullParser()
{
    deleteNamespaces();
}

void XMLPullParser::deleteNamespaces()
{
    NamespaceMap::iterator iter = _namespaces.begin();
    NamespaceMap::iterator end = _namespaces.end();
    for (; iter != end; ++iter) {
        while (iter->second) {
            Namespace* nspace = iter->second;
            iter->second = nspace->prev;
            delete nspace;
        }
    }
}

int XMLPullParser::setError(ErrorCode code)
{
    _error = code;

    getLog()->error("%s", getErrorDescription(code));

    return TokenError;
}

void XMLPullParser::warn(ErrorCode code)
{
    getLog()->warning("%s", getErrorDescription(code));
}

bool XMLPullParser::setErrorReturnFalseUnlessLenient(ErrorCode code)
{
    if (isLenient()) {
        warn(code);
        return true;
    }

    setError(code);
    return false;
}

void XMLPullParser::init(TextReader* textReader, const Options& options)
{
    _options = options;
    _textReader = textReader;
    _lastToken = TokenNone;

    if (_options.getHTMLEntities()) {
        setUserEntities(GetHTMLEntities());
    }

    if (_options.getHTMLMode()) {
        addEmptyElements(GetHTMLEmptyElements(), "");
    }
}

bool XMLPullParser::equalNames(const char* a, const char* b) const
{
    return (_options.getCaseInsensitiveNames()) ? ASCIIEqualIgnoringCase(a, b) : StringsEqual(a, b);
}

bool XMLPullParser::equalNamespaces(const char* a, const char* b)
{
    return ASCIIEqualIgnoringCase(a ? a : "", b ? b : "");
}

void XMLPullParser::addEmptyElements(ArrayView<const char*> elements, const char* namespaceForAll)
{
    const char* const* e = elements.end();
    NameAndNamespace nns = { 0, namespaceForAll };
    for (const char* const* p = elements.begin(); p != e; ++p) {
        nns.name = _stringTable.intern(*p);
        _emptyElements.push_back(nns);
    }
}

bool XMLPullParser::isEmptyElement(const char* name, const char* nspace) const
{
    // TODO: binary search (sort the array in addEmptyElements)
    std::vector<NameAndNamespace>::const_iterator i = _emptyElements.begin();
    std::vector<NameAndNamespace>::const_iterator e = _emptyElements.end();
    for (; i != e; ++i) {
        if (equalNames(i->name, name) && equalNamespaces(i->nspace, nspace)) {
            return true;
        }
    }

    return false;
}

void XMLPullParser::setUserEntities(ArrayView<const Entity> entities)
{
    _entities = entities;
}

int XMLPullParser::read()
{
    // Once we encounter an error, keep returning error.
    if (_lastToken == TokenError) {
        return TokenError;
    }

    _lastToken = read2();
    return _lastToken;
}

int XMLPullParser::read2()
{
    if (_emptyElement) {
        _emptyElement = false;
        _popElement = 1;
        return TokenEndElement;
    }

    if (_popElement) {
        --_popElement;
        popElement();

        if (_popElement != 0) {
            return TokenEndElement;
        }
    }

    _text.resize(0);

    int token;
    do {
        int c = _textReader->peekChar();
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return TokenError;
            }

            if (c == TextReader::EOFChar) {
                if (_elements.empty()) {
                    if (!_hadFirstTopLevelElement) {
                        if (!isLenient()) {
                            return setError(ErrorUnexpectedEndOfFile);
                        }

                        warn(ErrorUnexpectedEndOfFile);
                    }

                    return TokenEOF;
                }

                if (isLenient()) {
                    warn(ErrorUnexpectedEndOfFile);
                    // Auto-pop the remaining elements.
                    _popElement = (int)_elements.size();
                    setNameAndDetermineNamespace(_elements.back().name);
                    return TokenEndElement;
                }

                return setError(ErrorUnexpectedEndOfFile);
            }
        }

        if (c == '<' && (!inScript() || _textReader->hasString("</"))) {
            token = parseElement();
        } else {
            token = parseText();
        }
    } while (token == TokenNone);

    return token;
}

int XMLPullParser::next()
{
    for (;;) {
        int got = read();

        switch ((Token)got) {
        case TokenStartElement:
        case TokenText:
        case TokenEndElement:
            return got;

        case TokenEOF:
        case TokenError:
            return got;

        case TokenProcessingInstruction:
        case TokenComment:
        case TokenDocType:
        case TokenNone:
            break;
        }

        // Loop and skip to next token
    }
}

XMLPullParser::Attribute XMLPullParser::getAttribute(size_t index) const
{
    PRIME_ASSERT(index < getAttributeCount());
    return getAttributeCheckIndex((ptrdiff_t)index);
}

XMLPullParser::Attribute XMLPullParser::getAttributeCheckIndex(ptrdiff_t index) const
{
    Attribute a;
    if (index < 0) {
        a = emptyAttribute;
    } else {
        const InternedAttribute& ia = _elements.back().attributes[index];
        a.qualifiedName = ia.qualifiedName;
        a.nspace = ia.nspace;
        a.localName = ia.localName;
        a.value = _elements.back().values.data() + ia.valueOffset;
    }
    return a;
}

ptrdiff_t XMLPullParser::getAttributeIndex(StringView localName) const
{
    for (size_t i = 0; i != getAttributeCount(); ++i) {
        const InternedAttribute& ia = _elements.back().attributes[i];
        if (StringsEqual(ia.localName, localName)) {
            return (ptrdiff_t)i;
        }
    }

    return -1;
}

ptrdiff_t XMLPullParser::getAttributeIndex(StringView localName, StringView nspace) const
{
    for (size_t i = 0; i != getAttributeCount(); ++i) {
        const InternedAttribute& ia = _elements.back().attributes[i];
        if (StringsEqual(ia.localName, localName) && StringsEqual(ia.nspace, nspace)) {
            return (ptrdiff_t)i;
        }
    }

    return -1;
}

XMLPullParser::Attribute XMLPullParser::getAttribute(StringView localName) const
{
    return getAttributeCheckIndex(getAttributeIndex(localName));
}

XMLPullParser::Attribute XMLPullParser::getAttribute(StringView localName, StringView nspace) const
{
    return getAttributeCheckIndex(getAttributeIndex(localName, nspace));
}

int XMLPullParser::parseElement()
{
    PRIME_DEBUG_ASSERT(_textReader->peekChar(0) == '<');

    int c1 = _textReader->peekChar(1);

    int result;
    bool skipable = false;

    if (c1 == '?') {
        result = parseProcessingInstruction();
    } else if (c1 == '!') {
        result = parseExclamation();
    } else if (c1 == '/') {
        result = parseEndElement(&skipable);
    } else {
        result = parseStartElement(&skipable);
    }

    if (result == TokenError) {
        if (skipable) {
            // parseEndElement and parseStartElement will rewind
            _textReader->skipChar();
            return TokenNone;
        }
    }

    return result;
}

int XMLPullParser::parseExclamation()
{
    PRIME_DEBUG_ASSERT(_textReader->peekChar(0) == '<' && _textReader->peekChar(1) == '!');

    int c2 = _textReader->peekChar(2);
    int c3 = _textReader->peekChar(3);

    if (c2 == '-' && c3 == '-') {
        return parseComment();
    }

    if (c2 == '[' && c3 == 'C' && _textReader->hasString(cdataSectionHeader)) {
        return parseCDATA();
    }

    if (_textReader->hasString(doctypeHeader)) {
        return parseDocType();
    }

    if (!isStrict()) {
        warn(ErrorInvalidDocType);
        return parseDocType();
    }

    return setError(ErrorInvalidDocType);
}

bool XMLPullParser::addUnicodeChar(uint32_t n)
{
    if (n & UINT32_C(0x80000000)) {
        return setErrorReturnFalseUnlessLenient(ErrorInvalidCharacter);
    }

    uint8_t ch[7];
    size_t len = UTF8Encode(ch, (int32_t)n);
    ch[len] = 0;

    if (!ch[0]) {
        return setErrorReturnFalseUnlessLenient(ErrorInvalidCharacter);
    }

    _text.append((char*)ch, len);
    return true;
}

bool XMLPullParser::processHexCharacterNumber()
{
    PRIME_DEBUG_ASSERT(_textReader->peekChar() == '&' && _textReader->peekChar(1) == '#' && (_textReader->peekChar(2) == 'x' || _textReader->peekChar(2) == 'X'));
    _textReader->skipChars(3);

    uint32_t n = 0;
    int digitCount = 0;
    unsigned int digit = 0;

    for (int i = 0;; ++i) {
        int c = _textReader->peekChar(i);
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return false;
            }
            if (c == TextReader::EOFChar) {
                return setErrorReturnFalseUnlessLenient(ErrorUnexpectedEndOfFile);
            }
        }

        if (c == ';') {
            if (!digitCount) {
                return setErrorReturnFalseUnlessLenient(ErrorInvalidEntity);
            }

            _textReader->skipChars(i + 1);
            return addUnicodeChar(n);
        }

        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            return setErrorReturnFalseUnlessLenient(ErrorInvalidEntity);
        }

        if (digitCount == 8) {
            return setErrorReturnFalseUnlessLenient(ErrorInvalidEntity);
        }

        n = n * 16 + digit;
        if (n) {
            ++digitCount;
        }
    }
}

bool XMLPullParser::processCharacterNumber()
{
    PRIME_DEBUG_ASSERT(_textReader->peekChar() == '&' && _textReader->peekChar(1) == '#');
    if (_textReader->peekChar(2) == 'x' || (isLenient() && _textReader->peekChar(2) == 'X')) {
        return processHexCharacterNumber();
    }

    _textReader->skipChars(2);

    uint32_t n = 0;
    int digitCount = 0;

    for (int i = 0;; ++i) {
        int c = _textReader->peekChar(i);
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return false;
            }
            if (c == TextReader::EOFChar) {
                return setErrorReturnFalseUnlessLenient(ErrorUnexpectedEndOfFile);
            }
        }

        if (c == ';') {
            if (!digitCount) {
                return setErrorReturnFalseUnlessLenient(ErrorInvalidEntity);
            }

            _textReader->skipChars(i + 1);
            return addUnicodeChar(n);
        }

        if (c < '0' || c > '9' || digitCount > 8) {
            return setErrorReturnFalseUnlessLenient(ErrorInvalidEntity);
        }

        n = n * 10 + (c - '0');
        if (n) {
            ++digitCount;
        }
    }
}

bool XMLPullParser::processAmpersand()
{
    PRIME_DEBUG_ASSERT(_textReader->peekChar() == '&');

    if (_textReader->peekChar(1) == '#') {
        return processCharacterNumber();
    }

    bool lenient = isLenient();

    int len;
    bool invalid = false;
    for (len = 1;; ++len) {
        int peeked = _textReader->peekChar(len);

        if (peeked == ';') {
            ++len;
            break;
        }

        if (!IsNameChar(peeked, lenient, len == 1)) {
            invalid = true;
            break;
        }
    }

    if (invalid && isStrict()) {
        setError(ErrorInvalidEntity);
        return false;
    }

    if (!invalid) {
        // TODO: binary search (would need the entities to be copied by us and sorted)
        for (size_t i = 0; i != _entities.size(); ++i) {
            const Entity& e = _entities[i];
            if (strncmp(_textReader->getReadPointer(), e.token, len) == 0) {
                // Match!
                if (e.string) {
                    _text.append(e.string);
                } else {
                    if (!addUnicodeChar(e.entity)) {
                        return false;
                    }
                }

                _textReader->skipChars(len);
                return true;
            }
        }
    }

    // If we get here then it's an invalid or unknown entity reference. Treat it as literal text.
    _text += '&';
    _textReader->skipChar();

    // We can't produce an error here because as far as we know there's a valid ENTITY in the DocType.
    warn(ErrorUnknownEntity);
    return true;
}

void XMLPullParser::processCR()
{
    PRIME_DEBUG_ASSERT(_textReader->peekChar() == 13);

    // Windows style CRLF becomes plain LF.
    if (_textReader->peekChar(1) == 10) {
        _textReader->skipChars(2);
        _text += (char)10;
    } else {
        // Not CRLF, just CR - discard.
        _textReader->skipChar();
    }
}

void XMLPullParser::processLF()
{
    PRIME_DEBUG_ASSERT(_textReader->peekChar() == 10);

    // Old Mac style LFCR becomes plain LF.
    if (_textReader->peekChar(1) == 13) {
        _textReader->skipChars(2);
    } else {
        _textReader->skipChar();
    }

    _text += (char)10;
}

int XMLPullParser::parseText()
{
    bool isScript = inScript();

    for (;;) {
        int c = _textReader->peekChar();
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return TokenError;
            }
            if (c == TextReader::EOFChar) {
                break;
            }
        }

        if (c == '<') {
            if (!isScript || _textReader->hasString("</")) {
                break;
            }
        }

        if (processCRLF(c)) {
            continue;
        }

        if (!isValidText(c)) {
            return TokenError;
        }

        if (c == '&' && !isScript) {
            if (!processAmpersand()) {
                return TokenError;
            }

            continue;
        }

        if (isScript) {
            if (c == '\'' || c == '"') {
                if (!readScriptString()) {
                    return TokenError;
                }
                continue;

            } else if (c == '/') {
                int c2 = _textReader->peekChar(1);

                if (c2 == '/') {
                    if (!readScriptSingleLineComment()) {
                        return TokenError;
                    }
                    continue;

                } else if (c2 == '*') {
                    if (!readScriptMultiLineComment()) {
                        return TokenError;
                    }
                    continue;
                }
            }
        }

        if (c == ']' && _textReader->peekChar(1) == ']' && _textReader->peekChar(2) == '>') {
            if (isStrict()) {
                return setError(ErrorCDATATerminatorInText);
            }

            warn(ErrorCDATATerminatorInText);
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    if (_elements.empty() && !isTextEntirelyWhitespace()) {
        if (!isLenient()) {
            return setError(ErrorTextOutsideElement);
        }

        warn(ErrorTextOutsideElement);
    }

    _cdata = false;
    return TokenText;
}

bool XMLPullParser::readScriptString()
{
    int quote = _textReader->readChar();
    int lastC = quote;

    for (;;) {
        int c = _textReader->readChar();

        if (c < 0) {
            if (c == TextReader::EOFChar) {
                break;
            }

            return false;
        }

        _text += TextReader::intToChar(c);
        if (lastC != '\\' && c == quote) {
            break;
        }

        lastC = c;
    }

    return true;
}

bool XMLPullParser::readScriptSingleLineComment()
{
    for (;;) {
        int c = _textReader->peekChar();

        if (c < 0) {
            if (c == TextReader::EOFChar) {
                break;
            }

            return false;
        }

        if (processCRLF(c)) {
            break;
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    return true;
}

bool XMLPullParser::readScriptMultiLineComment()
{
    int lastC = ' ';

    _text += "/*";
    _textReader->skipChars(2);

    for (;;) {
        int c = _textReader->peekChar();

        if (c < 0) {
            if (c == TextReader::EOFChar) {
                break;
            }

            return false;
        }

        if (processCRLF(c)) {
            continue;
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();

        if (c == '/' && lastC == '*') {
            break;
        }

        lastC = c;
    }

    return true;
}

int XMLPullParser::parseCDATA()
{
    PRIME_DEBUG_ASSERT(_textReader->hasString(cdataSectionHeader));
    _textReader->skipChars(COUNTOF(cdataSectionHeader) - 1);

    for (;;) {
        int c = _textReader->peekChar();
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return TokenError;
            }
            if (c == TextReader::EOFChar) {
                if (isLenient()) {
                    warn(ErrorUnexpectedEndOfFile);
                    break;
                }

                return setError(ErrorUnexpectedEndOfFile);
            }
        }

        if (c == ']' && _textReader->peekChar(1) == ']' && _textReader->peekChar(2) == '>') {
            _textReader->skipChars(3);
            break;
        }

        if (processCRLF(c)) {
            continue;
        }

        if (!isValidText(c)) {
            return TokenError;
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    if (_elements.empty()) {
        if (!isLenient()) {
            return setError(ErrorTextOutsideElement);
        }

        warn(ErrorTextOutsideElement);
    }

    _cdata = true;
    return TokenText;
}

void XMLPullParser::popElement()
{
    _elements.pop_back();
    popNamespaces();
    if (!_elements.empty()) {
        setNameAndDetermineNamespace(_elements.back().name);
    }
}

void XMLPullParser::popNamespaces()
{
    NamespaceMap::iterator iter = _namespaces.begin();
    NamespaceMap::iterator end = _namespaces.end();
    for (; iter != end; ++iter) {
        Namespace* nspace = iter->second;
        if (nspace->depth > _elements.size()) {
            iter->second = nspace->prev;
            delete nspace;
        }
    }
}

void XMLPullParser::pushElement(const char* name)
{
    Element el;
    el.name = name;
    el.isScript = _options.getHTMLMode() && (ASCIIEqualIgnoringCase(name, "script") || ASCIIEqualIgnoringCase(name, "style"));
    _elements.push_back(el);
}

void XMLPullParser::setTopElementNamespace()
{
    Element& el = _elements.back();

    for (size_t i = 0; i != el.attributes.size(); ++i) {
        const InternedAttribute& a = el.attributes[i];

        if (strncmp(a.qualifiedName, "xmlns", 5) != 0) {
            continue;
        }

        const char* nspaceName = (a.qualifiedName[5] == ':') ? a.qualifiedName + 6 : "";

        const char* value = el.values.c_str() + a.valueOffset;

        value = _stringTable.intern(value);

        setNamespace(nspaceName, value);
    }

    setNameAndDetermineNamespace(el.name);
}

void XMLPullParser::setNamespace(const char* name, const char* value)
{
    Namespace* prev;

    NamespaceMap::const_iterator iter = _namespaces.find(name);
    if (iter != _namespaces.end()) {
        if (StringsEqual(iter->second->value, value)) {
            // Identical values, don't bother creating a new Namespace.
            return;
        }

        prev = iter->second;
    } else {
        prev = NULL;
    }

    ScopedPtr<Namespace> nspace(new Namespace);

    nspace->name = name;
    nspace->value = value;
    nspace->depth = _elements.size();
    nspace->prev = prev;

    _namespaces[name] = nspace.get();
    nspace.detach();
}

int XMLPullParser::parseStartElement(bool* skipable)
{
    TextReader::Marker marker(_textReader); // Rewind in case of skipable or element where it shouldn't be

    _textReader->skipChar(); // <

    if (!parseName(skipable)) {
        return TokenError;
    }

    const bool isTopLevelElement = _elements.empty();

    pushElement(_name);

    for (;;) {
        if (!skipWhitespace()) {
            return TokenError;
        }

        int c = _textReader->peekChar();

        if (c == '>') {
            _emptyElement = false;
            _textReader->skipChar();
            break;
        }

        if (c == '/' && _textReader->peekChar(1) == '>') {
            _emptyElement = true;
            _textReader->skipChars(2);
            break;
        }

        if (!parseAttribute(skipable)) {
            popElement();
            return TokenError;
        }
    }

    if (isTopLevelElement) {
        if (_hadFirstTopLevelElement) {
            if (!_options.getHTMLMode()) {
                return setError(ErrorMultipleTopLevelElements);
            }
            warn(ErrorMultipleTopLevelElements);
        }

        _hadFirstTopLevelElement = true;
    }

    setTopElementNamespace();

    Element& el = _elements.back();
    for (size_t i = 0; i != el.attributes.size(); ++i) {
        InternedAttribute& a = el.attributes[i];

        determineNamespaceAndLocalName(a.qualifiedName, &a.localName, &a.nspace);
        PRIME_DEBUG_ASSERT(a.localName);
    }

    // Check for duplicate attributes.
    if (!el.attributes.empty()) {
        for (size_t i = 0; i != el.attributes.size() - 1; ++i) {
            const InternedAttribute& a = el.attributes[i];

            for (size_t j = i + 1; j != el.attributes.size(); ++j) {
                const InternedAttribute& a2 = el.attributes[j];
                if (a.nspace == a2.nspace && a.localName == a2.localName) {
                    if (isStrict()) {
                        return setError(ErrorDuplicateAttribute);
                    } else {
                        warn(ErrorDuplicateAttribute);
                    }
                }
            }
        }
    }

    if (isEmptyElement(_localName, _namespace)) {
        _emptyElement = true;
    }

    if (!canElementBeHere()) {
        popElement();
        _popElement = 1;
        marker.rewind(); // Alternative to using a marker is to have the _popElement handling code in read2() push an element afterwards
        return TokenEndElement;
    }

    marker.release();
    return TokenStartElement;
}

bool XMLPullParser::canElementBeHere() const
{
    if (_options.getHTMLMode()) {
        // See http://www.w3.org/TR/html5/index.html#elements-1
        // TODO: col, colgroup, datalist, fieldset, legend, figure, figcaption, map, select,
        //       option, optgroup, ruby, rp, rt, rb, rtc, script??,  caption, template
        // TODO: What to do about multiple head/body?
        // Note that some cases are covered by htmlEmptyElements.
        if (ASCIIEqualIgnoringCase(_localName, "dd") || ASCIIEqualIgnoringCase(_localName, "dt")) {
            int dt = findAncestor("dt");
            int dd = findAncestor("dd");
            int dl = findAncestor("dl");
            if (dd > dl || dt > dl) {
                // Disallow dd/dt inside a dd/dt unless there's another dl
                return false;
            }

        } else if (ASCIIEqualIgnoringCase(_localName, "tr")) {
            int tr = findAncestor("tr");
            int td = findAncestor("td");
            int table = findAncestor("table");
            if (tr > table || td > table) {
                // Disallow td inside a tr/td unless there's another table
                return false;
            }

        } else if (ASCIIEqualIgnoringCase(_localName, "tbody") || ASCIIEqualIgnoringCase(_localName, "thead") || ASCIIEqualIgnoringCase(_localName, "tfoot")) {
            int table = findAncestor("table");
            int tr = findAncestor("tr");
            int td = findAncestor("td");
            int thead = findAncestor("thead");
            int tbody = findAncestor("tbody");
            int tfoot = findAncestor("tfoot");

            if (td > table || tr > table || thead > table || tfoot > table || tbody > table) {
                // thead, tfoot, tbody cannot occur inside each other unless there's another table
                return false;
            }

        } else if (ASCIIEqualIgnoringCase(_localName, "td")) {
            int td = findAncestor("td");
            int table = findAncestor("table");
            if (td > table) {
                // Disallow td inside a td unless there's another table
                return false;
            }

        } else if (ASCIIEqualIgnoringCase(_localName, "li")) {
            int list = Max(findAncestor("ol"), findAncestor("ul"));
            int li = findAncestor("li");
            if (li > list) {
                // Disallow li inside an li unless there's another ol/ul
                return false;
            }

        } else if (ASCIIEqualIgnoringCase(_localName, "param")) {
            int object = findAncestor("object");
            int param = findAncestor("param");
            if (param > object) {
                // Disallow param inside an param unless there's another object
                return false;
            }

        } else if (ASCIIEqualIgnoringCase(_localName, "source")) {
            int media = Max(findAncestor("video"), findAncestor("audio"));
            int source = findAncestor("source");
            if (source > media) {
                // Disallow source inside an source unless there's another video/audio
                return false;
            }

        } else if (ASCIIEqualIgnoringCase(_localName, "body")) {
            if (findAncestor("head") >= 0) {
                // Disallow body inside head. Good advice in general.
                return false;
            }

        } else if (ASCIIEqualIgnoringCase(_localName, "style")) {
            if (findAncestor("style") >= 0) {
                // Disallow style inside style.
                return false;
            }
        }
    }

    return true;
}

int XMLPullParser::findAncestor(const char* localName) const
{
    for (size_t i = _elements.size() - 1; i-- > 0;) {
        const Element& el = _elements[i];
        const char* ptr = strchr(el.name, ':');
        ptr = ptr ? ptr + 1 : el.name;
        if (ASCIIEqualIgnoringCase(ptr, localName)) {
            return (int)i;
        }
    }

    return -1;
}

int XMLPullParser::parseProcessingInstruction()
{
    PRIME_DEBUG_ASSERT(_textReader->peekChar(0) == '<' && _textReader->peekChar(1) == '?');
    _textReader->skipChars(2);

    if (!parseName(NULL)) {
        return TokenError;
    }

    if (!skipWhitespace()) {
        return TokenError;
    }

    _qualifiedName = _localName = _name;
    _namespace = NULL;

    _text.resize(0);

    int c;
    for (;;) {
        c = _textReader->peekChar();
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return TokenError;
            }
            if (c == TextReader::EOFChar) {
                if (isLenient()) {
                    warn(ErrorUnexpectedEndOfFile);
                    break;
                }

                return setError(ErrorUnexpectedEndOfFile);
            }
        }

        if (c == '?' && _textReader->peekChar(1) == '>') {
            break;
        }

        if (processCRLF(c)) {
            continue;
        }

        if (!isValidText(c)) {
            return TokenError;
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    if (c >= 0) {
        _textReader->skipChars(c == '>' ? 1 : 2);
    }
    return TokenProcessingInstruction;
}

bool XMLPullParser::isValidText(int c)
{
    if (c < ' ') {
        if (c != 13 && c != 10 && c != 9) {
            if (isStrict()) {
                return setErrorReturnFalse(ErrorInvalidCharacter);
            }

            warn(ErrorInvalidCharacter);
        }
    }

    return true;
}

bool XMLPullParser::parseName(bool* skipable)
{
    if (skipable) {
        *skipable = false;
    }

    _parseNameBuffer.resize(0);

    if (!skipWhitespaceIfLenient()) {
        return false;
    }

    bool lenient = isLenient();

    bool firstChar = true;
    for (;;) {
        int c = _textReader->peekChar();
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return false;
            }
            if (c == TextReader::EOFChar) {
                if (isLenient()) {
                    warn(ErrorUnexpectedEndOfFile);
                    break;
                }

                return setErrorReturnFalse(ErrorUnexpectedEndOfFile);
            }
        }

        if (firstChar) {
            if (!IsNameStartChar(c, lenient)) {
                if (skipable) {
                    warn(ErrorIllegalName);
                    *skipable = true;
                    return false;
                } else {
                    return setErrorReturnFalse(ErrorIllegalName);
                }
            }

            firstChar = false;
        } else if (!IsNameChar(c, lenient)) {
            break;
        }

        _parseNameBuffer += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    // This will only allocate memory the first time a name is encountered.
    _name = _stringTable.intern(_parseNameBuffer.c_str());

    return true;
}

bool XMLPullParser::parseUnquotedAttributeValue()
{
    _text.resize(0);

    bool lenient = isLenient(); // will probably always be true if we've reached this method!

    for (;;) {
        int c = _textReader->peekChar();
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return false;
            }
            if (c == TextReader::EOFChar) {
                if (isLenient()) {
                    warn(ErrorUnexpectedEndOfFile);
                    break;
                }

                return setErrorReturnFalse(ErrorUnexpectedEndOfFile);
            }
        }

        if (!IsXMLUnquotedAttributeValueChar(c, lenient)) {
            break;
        }

        if (c == '/' && _textReader->peekChar(1) == '>') {
            break;
        }

        if (processCRLF(c)) {
            continue;
        }

        if (!isValidText(c)) {
            return false;
        }

        if (c == '&') {
            if (!processAmpersand()) {
                return false;
            }

            continue;
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    return true;
}

bool XMLPullParser::parseAttributeValue()
{
    int quot = _textReader->peekChar();
    if (quot != '"') {
        if (quot != '\'') {
            if (!isLenient()) {
                setError(ErrorExpectedQuote);
                return false;
            }
            // Allow unquoted attribute values for HTML
            if (!_options.getHTMLMode()) {
                warn(ErrorExpectedQuote);
            }
            return parseUnquotedAttributeValue();
        } else {
            // Allow ' for HTML
            if (!_options.getHTMLMode()) {
                warn(ErrorExpectedQuote);
            }
        }
    }

    _textReader->skipChar();

    _text.resize(0);

    for (;;) {
        int c = _textReader->peekChar();
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return false;
            }
            if (c == TextReader::EOFChar) {
                if (isLenient()) {
                    warn(ErrorUnexpectedEndOfFile);
                    break;
                }

                return setErrorReturnFalse(ErrorUnexpectedEndOfFile);
            }
        }

        if (c == quot) {
            _textReader->skipChar();
            break;
        }

        if (processCRLF(c)) {
            continue;
        }

        if (!isValidText(c)) {
            return false;
        }

        if (c == '&') {
            if (!processAmpersand()) {
                return false;
            }

            continue;
        }

        // # is sometimes considered invalid, but I can't find consensus
        if (strchr("<&", TextReader::intToChar(c))) {
            if (isStrict()) {
                return setErrorReturnFalse(ErrorInvalidAttributeValue);
            }

            // Allow these characters in HTML.
            if (!_options.getHTMLMode()) {
                warn(ErrorInvalidAttributeValue);
            }
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    return true;
}

bool XMLPullParser::parseAttribute(bool* skipable)
{
    // parseName initialises skipable
    if (!parseName(skipable)) {
        return false;
    }

    InternedAttribute a;
    a.qualifiedName = _name;

    if (!skipWhitespaceIfLenient()) {
        return false;
    }

    if (_textReader->peekChar() != '=') {
        if (!isLenient()) {
            setError(ErrorExpectedEquals);
            return false;
        }
        // Allow attributes with no value in HTML.
        if (!_options.getHTMLMode()) {
            warn(ErrorExpectedEquals);
        }
        _text.resize(0);
    } else {
        _textReader->skipChar();

        if (!skipWhitespaceIfLenient()) {
            return false;
        }

        if (!parseAttributeValue()) {
            return false;
        }
    }

    Element& el = _elements.back();

    a.valueOffset = el.values.size();
    el.values.append(_text.c_str(), _text.size() + 1); // include the NUL
    el.attributes.push_back(a);

    return true;
}

bool XMLPullParser::skipWhitespace()
{
    bool lenient = isLenient();

    for (;;) {
        int c = _textReader->peekChar();
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return false;
            }
            if (c == TextReader::EOFChar) {
                break;
            }
        }

        if (!IsXMLWhitespace(c, lenient)) {
            break;
        }

        _textReader->skipChar();
    }

    return true;
}

bool XMLPullParser::skipWhitespaceIfLenient()
{
    bool lenient = isLenient();
    bool skipped = false;

    for (;;) {
        int c = _textReader->peekChar();
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return false;
            }
            if (c == TextReader::EOFChar) {
                break;
            }
        }

        if (!IsXMLWhitespace(c, lenient)) {
            break;
        }

        _textReader->skipChar();
        skipped = true;
    }

    if (skipped) {
        warn(ErrorUnexpectedWhitespace);
    }

    return true;
}

int XMLPullParser::parseEndElement(bool* skipable)
{
    TextReader::Marker marker(_textReader); // Rewind in case of skipable

    PRIME_DEBUG_ASSERT(_textReader->peekChar(0) == '<' && _textReader->peekChar(1) == '/');
    _textReader->skipChars(2);

    if (!skipWhitespaceIfLenient()) {
        return TokenError;
    }

    if (_elements.empty()) {
        if (!isLenient()) {
            return setError(ErrorUnexpectedEndElement);
        }

        warn(ErrorUnexpectedEndElement);
        return TokenNone;
    }

    if (!parseName(skipable)) {
        return TokenError;
    }

    int popCount = 1;

    if (_name != _elements.back().name) {
        if (!isLenient()) {
            return setError(ErrorMismatchedEndElement);
        }

        warn(ErrorMismatchedEndElement);

        setNameAndDetermineNamespace(_name);

        // See if we can find a match.
        popCount = 0;
        size_t i = _elements.size();
        while (i--) {
            // TODO!
            // if (equalNames(_elements[i].name, _name) && equalNamespaces(_elements[i].nspace, _namespace)) {
            if (equalNames(_elements[i].name, _name)) {
                // Found it!
                popCount = (int)(_elements.size() - i);
                break;
            }
        }
    }

    setNameAndDetermineNamespace(_elements.back().name);

    if (!skipWhitespace()) {
        return TokenError;
    }

    if (_textReader->peekChar() != '>') {
        if (!isLenient()) {
            return setError(ErrorExpectedRightAngleBracket);
        }

        warn(ErrorExpectedRightAngleBracket);
    } else {
        _textReader->skipChar();
    }

    marker.release();

    _popElement = popCount;
    return popCount ? TokenEndElement : TokenNone;
}

int XMLPullParser::parseComment()
{
    PRIME_DEBUG_ASSERT(_textReader->peekChar(0) == '<' && _textReader->peekChar(1) == '!' && _textReader->peekChar(2) == '-' && _textReader->peekChar(2) == '-');

    _textReader->skipChars(4);

    for (;;) {
        int c = _textReader->peekChar();
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return TokenError;
            }
            if (c == TextReader::EOFChar) {
                if (isLenient()) {
                    warn(ErrorUnexpectedEndOfFile);
                    break;
                }

                return setError(ErrorUnexpectedEndOfFile);
            }
        }

        if (c == '-' && _textReader->peekChar(1) == '-') {
            if (_textReader->peekChar(2) == '>') {
                _textReader->skipChars(3);
                break;
            }

            if (isStrict()) {
                return setError(ErrorIncorrectlyTerminatedComment);
            }
        }

        if (!isValidText(c)) {
            return TokenError;
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }

    return TokenComment;
}

int XMLPullParser::parseDocType()
{
    PRIME_DEBUG_ASSERT(_textReader->peekChar(0) == '<' && _textReader->peekChar(1) == '!');

    // Read the entire DocType as raw text, without parsing it.

    _textReader->skipChars(2);

    int nest = 1;

    for (;;) {
        int c = _textReader->peekChar();
        if (c < 0) {
            if (c == TextReader::ErrorChar) {
                readFailed();
                return TokenError;
            }
            if (c == TextReader::EOFChar) {
                if (isLenient()) {
                    warn(ErrorUnexpectedEndOfFile);
                    return TokenDocType;
                }

                return setError(ErrorUnexpectedEndOfFile);
            }
        }

        if (c == '>') {
            if (!--nest) {
                _textReader->skipChar();
                return TokenDocType;
            }
        } else if (c == '<') {
            if (_textReader->hasString("<!--")) {
                std::string text2;
                text2.swap(_text);
                int token = parseComment();
                if (token < TokenNone) {
                    return token;
                }
                text2.swap(_text);
                _text += "<!--";
                _text += text2;
                _text += "-->";
                continue;
            } else {
                ++nest;
            }
        } else if (c == '\'' || c == '"') {
            int quot = c;
            _text += TextReader::intToChar(c);
            _textReader->skipChar();

            do {
                c = _textReader->readChar();
                if (c < 0) {
                    if (c == TextReader::ErrorChar) {
                        readFailed();
                        return TokenError;
                    }
                    if (c == TextReader::EOFChar) {
                        if (isLenient()) {
                            warn(ErrorUnexpectedEndOfFile);
                            return TokenDocType;
                        }

                        return setError(ErrorUnexpectedEndOfFile);
                    }
                }

                _text += TextReader::intToChar(c);
            } while (c != quot);
            continue;
        }

        _text += TextReader::intToChar(c);
        _textReader->skipChar();
    }
}

void XMLPullParser::determineNamespaceAndLocalName(const char* name, const char** localName, const char** nspace)
{
    // name should already have been interned.
    PRIME_DEBUG_ASSERT(!*name || _stringTable.intern(name) == name);

    const char* colon = strchr(name, ':');
    if (!colon) {
        *localName = name;
        *nspace = findNamespace();
    } else {
        ptrdiff_t colonPos = colon - name;
        *localName = name + colonPos + 1;

        std::string prefix(name, colonPos);
        *nspace = findNamespace(prefix.c_str());
    }
}

void XMLPullParser::setNameAndDetermineNamespace(const char* name)
{
    // name should already have been interned.
    PRIME_DEBUG_ASSERT(!*name || _stringTable.intern(name) == name);

    _qualifiedName = name;
    determineNamespaceAndLocalName(name, &_localName, &_namespace);
}

const char* XMLPullParser::findNamespace()
{
    return findNamespace("");
}

const char* XMLPullParser::findNamespace(const char* prefix)
{
    NamespaceMap::const_iterator iter = _namespaces.find(prefix);

    const char* value;

    if (iter == _namespaces.end()) {
        if (*prefix && !StringsEqual(prefix, "xmlns") && !StringsEqual(prefix, "xml")) {
            // Don't warn for the default namespace or xmlns.
            getLog()->warning("%s: %s", getErrorDescription(ErrorUnknownNamespace), prefix);
        }
        value = NULL;
    } else {
        value = iter->second->value;
    }

#if TEST_NAMESPACE_MAP
    const char* secondOpinion = *prefix ? findNamespaceOld(prefix) : findNamespaceOld();
    PRIME_ASSERT(secondOpinion != NULL || value == NULL);
    PRIME_ASSERT(StringsEqual(value ? value : "", secondOpinion ? secondOpinion : ""));
#endif

    return value;
}

const char* XMLPullParser::findNamespaceOld()
{
    std::vector<Element>::iterator ie = _elements.end();
    std::vector<Element>::iterator ee = _elements.begin();
    while (ie-- != ee) {
        std::vector<InternedAttribute>::iterator ia = ie->attributes.begin();
        std::vector<InternedAttribute>::iterator ea = ie->attributes.end();

        for (; ia != ea; ++ia) {
            if (StringsEqual(ia->qualifiedName, "xmlns")) {
                return _stringTable.intern(ie->values.data() + ia->valueOffset);
            }
        }
    }

    return 0;
}

const char* XMLPullParser::findNamespaceOld(const char* prefix)
{
    static const char xmlnsColon[] = "xmlns:";
    static const size_t xmlnsColonLength = sizeof(xmlnsColon) - 1;
    size_t totalLength = xmlnsColonLength + strlen(prefix);

    std::vector<Element>::iterator ie = _elements.end();
    std::vector<Element>::iterator ee = _elements.begin();
    while (ie != ee) {
        --ie;
        std::vector<InternedAttribute>::iterator ia = ie->attributes.begin();
        std::vector<InternedAttribute>::iterator ea = ie->attributes.end();

        for (; ia != ea; ++ia) {
            if (strlen(ia->qualifiedName) == totalLength && strncmp(ia->qualifiedName, xmlnsColon, xmlnsColonLength) == 0) {
                if (StringsEqual(ia->qualifiedName + xmlnsColonLength, prefix)) {
                    return _stringTable.intern(ie->values.data() + ia->valueOffset);
                }
            }
        }
    }

    return 0;
}

const char* XMLPullParser::readWholeText(const char* elementDescription)
{
    _wholeText.resize(0);

    if (_lastToken == TokenText) {
        _wholeText = _text;
    }

    for (;;) {
        int token = read();
        if (token == TokenError) {
            return NULL;
        }

        if (token == TokenComment) {
            continue;
        }

        if (token == TokenText) {
            _wholeText.append(_text.data(), _text.size());
            continue;
        }

        if (token == TokenEndElement) {
            break;
        }

        getLog()->error(PRIME_LOCALISE("Unexpected %s in %s element."), getTokenDescription(token), elementDescription);
        _error = ErrorExpectedText;
        return NULL;
    }

    return _wholeText.c_str();
}

const char* XMLPullParser::readWholeTextTrimmed(const char* elementDescription)
{
    const char* got = readWholeText(elementDescription);
    if (!got) {
        return NULL;
    }

    size_t leading = CountLeadingWhitespace(_wholeText.c_str(), _wholeText.size(), isLenient());

    _wholeText.erase(0, leading);

    size_t trailing = CountTrailingWhitespace(_wholeText.c_str(), _wholeText.size(), isLenient());

    _wholeText.resize(_wholeText.size() - trailing);

    return _wholeText.c_str();
}

bool XMLPullParser::skipElement()
{
    if (_lastToken != TokenStartElement) {
        return true;
    }

    PRIME_ASSERT(!_elements.empty());

    for (int nest = 1;;) {
        int token = read();
        if (token == TokenError) {
            return false;
        }

        // TokenEOF shouldn't happen since we think we're inside an element.
        if (token == TokenEOF) {
            return setErrorReturnFalse(ErrorUnexpectedEndOfFile);
        }

        if (token == TokenEndElement) {
            if (!--nest) {
                return true;
            }
        }

        if (token == TokenStartElement) {
            ++nest;
        }
    }
}

bool XMLPullParser::skipEmptyElement()
{
    if (_lastToken != TokenStartElement) {
        return true;
    }

    PRIME_ASSERT(!_elements.empty());

    for (;;) {
        int token = read();
        if (token == TokenError) {
            return false;
        }

        // TokenEOF shouldn't happen since we think we're inside an element.
        if (token == TokenEOF) {
            return setErrorReturnFalse(ErrorUnexpectedEndOfFile);
        }

        if (token == TokenEndElement) {
            return true;
        }

        if (token == TokenStartElement || (token == TokenText && !IsXMLWhitespace(getText(), isLenient()))) {
            // TODO: In Lenient mode, this could just be a warning?
            return setErrorReturnFalse(ErrorExpectedEmptyElement);
        }
    }
}

bool XMLPullParser::isTextEntirelyWhitespace() const
{
    return IsXMLWhitespace(_text, isLenient());
}

//
// XMLPullParser::StringTable
//

XMLPullParser::StringTable::~StringTable()
{
    for (StringsSet::iterator iter = _strings.begin(); iter != _strings.end(); ++iter) {
        delete[] * iter;
    }

    _strings.clear();
}

const char* XMLPullParser::StringTable::intern(const char* string)
{
    StringsSet::iterator iter = _strings.lower_bound((char*)string);
    if (iter != _strings.end() && StringsEqual(*iter, string)) {
        return *iter;
    }

    StringsSet::const_iterator insertion = _strings.insert(iter, NewString(string));
    return *insertion;
}

const char* XMLPullParser::StringTable::intern(const char* string, size_t len)
{
    _internBuffer.assign(string, string + len);
    return intern(_internBuffer.c_str());
}
}
