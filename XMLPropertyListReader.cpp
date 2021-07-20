// Copyright 2000-2021 Mark H. P. Lord

#include "XMLPropertyListReader.h"
#include "StringUtils.h"
#include "TextEncoding.h"

namespace Prime {

XMLPropertyListReader::XMLPropertyListReader()
{
}

Value XMLPropertyListReader::read(Stream* stream, Log* log, size_t bufferSize)
{
    TextReader textReader;
    textReader.setLog(log);
    textReader.setStream(stream, bufferSize);

    return read(&textReader);
}

Value XMLPropertyListReader::read(TextReader* textReader)
{
    XMLPullParser xmlParser;
    xmlParser.init(textReader);

    Value result;
    bool readAnElement = false;
    bool foundPlistElement = false;

    for (;;) {
        int token = xmlParser.read();

        if (token == XMLPullParser::TokenError) {
            return undefined;
        }

        if (token == XMLPullParser::TokenEOF) {
            return undefined;
        }

        if (token == XMLPullParser::TokenEndElement) {
            break;
        }

        if (token == XMLPullParser::TokenStartElement) {
            if (!foundPlistElement) {
                if (!StringsEqual(xmlParser.getName(), "plist")) {
                    xmlParser.getLog()->error(PRIME_LOCALISE("Not an XML property list, expected plist element, got: %s"), xmlParser.getName());
                    return undefined;
                }

                foundPlistElement = true;
            } else {
                if (readAnElement) {
                    xmlParser.getLog()->error(PRIME_LOCALISE("XML property list contain multiple values."));
                    return undefined;
                }

                result = readElement(&xmlParser);
                if (result.isUndefined()) {
                    return result;
                }

                readAnElement = true;
            }
        } else if (!xmlParser.skipElement()) {
            return undefined;
        }
    }

    if (!readAnElement) {
        xmlParser.getLog()->error(PRIME_LOCALISE("XML property list is empty."));
        return undefined;
    }

    return result;
}

Value XMLPropertyListReader::readElement(XMLPullParser* fromXMLParser)
{
    _xmlParser = fromXMLParser;

    if (StringsEqual(_xmlParser->getName(), "true")) {
        return readBool(true);
    }

    if (StringsEqual(_xmlParser->getName(), "false")) {
        return readBool(false);
    }

    if (StringsEqual(_xmlParser->getName(), "integer")) {
        return readInteger();
    }

    if (StringsEqual(_xmlParser->getName(), "real")) {
        return readReal();
    }

    if (StringsEqual(_xmlParser->getName(), "string")) {
        return readString();
    }

    if (StringsEqual(_xmlParser->getName(), "date")) {
        return readDate();
    }

    if (StringsEqual(_xmlParser->getName(), "data")) {
        return readData();
    }

    if (StringsEqual(_xmlParser->getName(), "array")) {
        return readArray();
    }

    if (StringsEqual(_xmlParser->getName(), "dict")) {
        return readDict();
    }

    _xmlParser->getLog()->error(PRIME_LOCALISE("Unknown element in XML property list: %s"), _xmlParser->getName());
    return undefined;
}

Value XMLPropertyListReader::readBool(bool value)
{
    if (!_xmlParser->skipEmptyElement()) {
        return undefined;
    }

    return Value(value);
}

Value XMLPropertyListReader::readInteger()
{
    const char* text = _xmlParser->readWholeTextTrimmed("integer");
    if (!text) {
        return undefined;
    }

    Value::Integer n;
    if (!StringToInt<Value::Integer>(text, n)) {
        _xmlParser->getLog()->error(PRIME_LOCALISE("XML property list integer element does not contain an integer."));
        return undefined;
    }

    return Value(n);
}

Value XMLPropertyListReader::readReal()
{
    const char* text = _xmlParser->readWholeTextTrimmed("real");
    if (!text) {
        return undefined;
    }

    Value::Real n;
    if (!StringToReal<Value::Real>(text, n)) {
        _xmlParser->getLog()->error(PRIME_LOCALISE("XML property list real element does not contain a number."));
        return undefined;
    }

    return Value(n);
}

Value XMLPropertyListReader::readString()
{
    const char* text = _xmlParser->readWholeText("string");
    if (!text) {
        return undefined;
    }

    return Value(text);
}

Value XMLPropertyListReader::readDate()
{
    const char* text = _xmlParser->readWholeTextTrimmed("date");
    if (!text) {
        return undefined;
    }

    if (Optional<UnixTime> unixTime = DateTime::parseISO8601UnixTime(text)) {
        return *unixTime;
    }

    _xmlParser->getLog()->error(PRIME_LOCALISE("Malformed date in XML property list."));
    return undefined;
}

Value XMLPropertyListReader::readData()
{
    const char* text = _xmlParser->readWholeText("data");
    if (!text) {
        return undefined;
    }

    Data data;
    if (!Base64DecodeAppend(data.string(), text)) {
        _xmlParser->getLog()->error(PRIME_LOCALISE("Malformed Base-64 data in XML property list."));
        return undefined;
    }

    return Value(PRIME_MOVE(data));
}

Value XMLPropertyListReader::readArray()
{
    Value::Vector array;

    for (;;) {
        int token = _xmlParser->read();
        if (token == XMLPullParser::TokenError) {
            return undefined;
        }

        if (token == XMLPullParser::TokenEndElement) {
            break;
        }

        if (token == XMLPullParser::TokenStartElement) {
            Value element = readElement(_xmlParser);
            if (element.isUndefined()) {
                return undefined;
            }

            array.push_back(PRIME_MOVE(element));
            continue;
        }

        if (token == XMLPullParser::TokenText && !_xmlParser->isTextEntirelyWhitespace()) {
            _xmlParser->getLog()->error(PRIME_LOCALISE("XML property list array contains text."));
            return undefined;
        }

        if (!_xmlParser->skipElement()) {
            return undefined;
        }
    }

    return array;
}

Value XMLPropertyListReader::readDict()
{
    Value::Dictionary dict;

    Value* slot = NULL;

    for (;;) {
        int token = _xmlParser->read();
        if (token == XMLPullParser::TokenError) {
            return undefined;
        }

        if (token == XMLPullParser::TokenEndElement) {
            break;
        }

        if (token == XMLPullParser::TokenStartElement) {
            if (!slot) {
                if (!StringsEqual(_xmlParser->getName(), "key")) {
                    _xmlParser->getLog()->error(PRIME_LOCALISE("XML property list dictionary should contain alternating keys and values."));
                    return undefined;
                }

                const char* text = _xmlParser->readWholeText("key");
                if (!text) {
                    return undefined;
                }

                slot = &dict.access(text);
                continue;
            } else {
                *slot = readElement(_xmlParser);
                if (slot->isUndefined()) {
                    return undefined;
                }

                slot = NULL;
                continue;
            }
        }

        if (token == XMLPullParser::TokenText && !_xmlParser->isTextEntirelyWhitespace()) {
            _xmlParser->getLog()->error(PRIME_LOCALISE("XML property list dictionary contains text."));
            return undefined;
        }

        if (!_xmlParser->skipElement()) {
            return undefined;
        }
    }

    return dict;
}
}
