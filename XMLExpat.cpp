// Copyright 2000-2021 Mark H. P. Lord

#include "XMLExpat.h"

namespace Prime {

XMLExpat::XMLExpat()
    : _userData(NULL)
    , _startHandler(NULL)
    , _endHandler(NULL)
{
}

XMLExpat::~XMLExpat()
{
}

void XMLExpat::setElementHandler(StartElementHandler startElementHandler, EndElementHandler endElementHandler)
{
    _startHandler = startElementHandler;
    _endHandler = endElementHandler;
}

void XMLExpat::setCharacterDataHandler(CharacterDataHandler characterDataHandler)
{
    _characterHandler = characterDataHandler;
}

bool XMLExpat::run(XMLPullParser& parser)
{
    _parser = &parser;

    std::vector<const char*> atts;

    for (;;) {
        XMLPullParser::Token token = static_cast<XMLPullParser::Token>(parser.read());

        switch (token) {
        case XMLPullParser::TokenEOF:
            return true;

        case XMLPullParser::TokenError:
            return false;

        case XMLPullParser::TokenStartElement:
            if (_startHandler) {
                atts.resize(parser.getAttributeCount() * 2 + 1);
                for (size_t i = 0; i != parser.getAttributeCount(); ++i) {
                    XMLPullParser::Attribute att = parser.getAttribute(i);
                    atts[i * 2] = att.localName;
                    atts[i * 2 + 1] = att.value;
                }
                atts.back() = NULL;
                (*_startHandler)(_userData, parser.getName(), &atts[0]);
            }
            break;

        case XMLPullParser::TokenEndElement:
            if (_endHandler) {
                (*_endHandler)(_userData, parser.getName());
            }
            break;

        case XMLPullParser::TokenText:
            if (_characterHandler) {
                (*_characterHandler)(_userData, parser.getText().data(), Narrow<int>(parser.getTextLength()));
            }
            break;

        case XMLPullParser::TokenComment:
            break;

        case XMLPullParser::TokenProcessingInstruction:
            break;

        case XMLPullParser::TokenDocType:
            break;

        case XMLPullParser::TokenNone:
            PRIME_UNREACHABLE();
            break;
        }
    }

    return true;
}
}
