// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_XMLTESTS_H
#define PRIME_XMLTESTS_H

#include "XMLNodeReader.h"
#include "XMLNodeWriter.h"
#include "XMLPullParser.h"

namespace Prime {

namespace XMLTestsPrivate {

    inline void XMLPullParserTest1(Log* log)
    {
        static const char xml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                  "<messages xmlns=\"message-archve\">\n"
                                  "  <note id=\"501\">\n"
                                  "    <to>Tove</to>\n"
                                  "    <from>Jani</from>\n"
                                  "    <heading>Reminder</heading>\n"
                                  "    <body>Don't forget me this weekend!</body>\n"
                                  "  </note>\n"
                                  "  <note id=\"502\">\n"
                                  "    <to>Jani</to>\n"
                                  "    <from>Tove</from>\n"
                                  "    <heading>Re: Reminder</heading>\n"
                                  "    <body>I will not</body>\n"
                                  "  </note>\n"
                                  "</messages>\n";

        TextReader textReader;
        textReader.setLog(log);
        textReader.setText(xml);

        XMLPullParser xmlReader;
        xmlReader.init(&textReader, XMLPullParser::Options());

        for (;;) {
            int token = xmlReader.read();

            switch (token) {
            case XMLPullParser::TokenError:
                RuntimeError("ERROR");
                break;
            case XMLPullParser::TokenEOF:
                log->output("EOF\n");
                break;
            case XMLPullParser::TokenNone:
                log->output("None!\n");
                break;
            case XMLPullParser::TokenText:
                log->output("Text: %s\n", xmlReader.getText().c_str());
                break;
            case XMLPullParser::TokenProcessingInstruction:
                log->output("Processing instruction: %s\n", xmlReader.getText().c_str());
                break;
            case XMLPullParser::TokenStartElement:
                log->output("Start element: name=%s space=%s prefixed=%s\n", xmlReader.getName(), xmlReader.getNamespace(), xmlReader.getQualifiedName());
                {
                    for (size_t i = 0; i != xmlReader.getAttributeCount(); ++i) {
                        XMLPullParser::Attribute attr = xmlReader.getAttribute(i);
                        log->output("Attribute: name=%s space=%s prefixed=%s value=\"%s\"\n", attr.localName, attr.nspace, attr.qualifiedName, attr.value);
                    }
                }
                break;
            case XMLPullParser::TokenEndElement:
                log->output("End element: name=%s space=%s prefixed=%s\n", xmlReader.getName(), xmlReader.getNamespace(), xmlReader.getQualifiedName());
                break;
            case XMLPullParser::TokenComment:
                log->output("Comment: %s\n", xmlReader.getText().c_str());
                break;
            case XMLPullParser::TokenDocType:
                log->output("DocType: %s\n", xmlReader.getText().c_str());
                break;
            default:
                log->output("Unknown token type %d!\n", token);
            }

            if (token == XMLPullParser::TokenError || token == XMLPullParser::TokenEOF) {
                break;
            }
        }
    }

    inline void XMLPullParserTest2(Log* log)
    {
        static const char xml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                  "<messages xmlns=\"message-archve\">\n"
                                  "  <note id='501'>\n" // ' instead of "
                                  "    <to>Tove</to>\n"
                                  "    <from>Jani</from>\n"
                                  "    <heading>Reminder</heading>\n"
                                  "    <body>Don't forget me this weekend!</body>\n"
                                  "  </note>\n"
                                  "  < note   id  =  \"502\" >\n"
                                  "    <to>Jani\n" // Lots of missing end tags from here on
                                  "    <from>Tove\n"
                                  "    <heading>Re: Reminder\n"
                                  "    <body>I will not\n"
                                  "    </non-existent>\n" // non-existent
                                  "  </note>\n"
            // "</messages>\n"
            ;

        TextReader textReader;
        textReader.setLog(log);
        textReader.setText(xml);

        XMLPullParser xmlReader;
        XMLPullParser::Options xmlReaderOptions;
        xmlReaderOptions.setConformance(XMLPullParser::ConformanceLenient);
        xmlReader.init(&textReader, xmlReaderOptions);

        for (;;) {
            int token = xmlReader.read();

            switch (token) {
            case XMLPullParser::TokenError:
                RuntimeError("ERROR");
                break;
            case XMLPullParser::TokenEOF:
                log->output("EOF\n");
                break;
            case XMLPullParser::TokenNone:
                log->output("None!\n");
                break;
            case XMLPullParser::TokenText:
                log->output("Text: %s\n", xmlReader.getText().c_str());
                break;
            case XMLPullParser::TokenProcessingInstruction:
                log->output("Processing instruction: %s\n", xmlReader.getText().c_str());
                break;
            case XMLPullParser::TokenStartElement:
                log->output("Start element: name=%s space=%s prefixed=%s\n", xmlReader.getName(), xmlReader.getNamespace(), xmlReader.getQualifiedName());
                {
                    for (size_t i = 0; i != xmlReader.getAttributeCount(); ++i) {
                        XMLPullParser::Attribute attr = xmlReader.getAttribute(i);
                        log->output("Attribute: name=%s space=%s prefixed=%s value=\"%s\"\n", attr.localName, attr.nspace, attr.qualifiedName, attr.value);
                    }
                }
                break;
            case XMLPullParser::TokenEndElement:
                log->output("End element: name=%s space=%s prefixed=%s\n", xmlReader.getName(), xmlReader.getNamespace(), xmlReader.getQualifiedName());
                break;
            case XMLPullParser::TokenComment:
                log->output("Comment: %s\n", xmlReader.getText().c_str());
                break;
            case XMLPullParser::TokenDocType:
                log->output("DocType: %s\n", xmlReader.getText().c_str());
                break;
            default:
                log->output("Unknown token type %d!\n", token);
            }

            if (token == XMLPullParser::TokenError || token == XMLPullParser::TokenEOF) {
                break;
            }
        }
    }

    inline void XMLNodeTest(Log* log)
    {
        {
            static const char xml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                      "<messages xmlns=\"message-archve\">\n"
                                      "  <note id=\"501\">\n"
                                      "    <to>Tove</to>\n"
                                      "    <from>Jani</from>\n"
                                      "    <heading>Reminder</heading>\n"
                                      "    <body>Don't forget me this weekend!</body>\n"
                                      "  </note>\n"
                                      "  <note id=\"502\">\n"
                                      "    <to>Jani</to>\n"
                                      "    <from>Tove</from>\n"
                                      "    <heading>Re: Reminder</heading>\n"
                                      "    <body>I will not</body>\n"
                                      "  </note>\n"
                                      "</messages>\n";

            TextReader textReader;
            textReader.setText(xml);
            textReader.setLog(log);

            XMLPullParser xmlReader;
            xmlReader.init(&textReader);

            XMLNodeReader nodeReader;
            PRIME_TEST(nodeReader.readDocument(&xmlReader));

            StringStream stream;
            XMLNodeWriter writer(XMLNodeWriter::Options(), &stream, log, 1024);

            PRIME_TEST(writer.writeDocument(nodeReader.getDocument(), true));

            log->output("%s\n", stream.c_str());
        }

        {
            StringStream stream;

            {
                XMLNode root;
                root.setName("message-archive", "messages");
                root.setAttribute("message-archive", "version", "1.0");
                root.setAttribute("other-namespace", "version", "1.0", "ons");
                root.setAttribute("yet-another-namespace", "version", "1.0");
                root.setAttribute("", "xmlns:hooray", "awesome-namespace");

                XMLNode* child = root.addChild("message-archive", "note");
                child->addAttribute("awesome-namespace", "thingyumybob", "yes");
                child->addTextChild("Hello, world");

                XMLNodeWriter writer(XMLNodeWriter::Options(), &stream, log, 1024);

                PRIME_TEST(writer.writeDocument(&root, false));

                log->output("%s\n", stream.c_str());
            }

            {
                TextReader textReader;
                textReader.setText(stream.getString());
                textReader.setLog(log);

                XMLPullParser xmlReader;
                xmlReader.init(&textReader);

                XMLNodeReader nodeReader;
                PRIME_TEST(nodeReader.readDocument(&xmlReader));

                StringStream stream2;
                XMLNodeWriter writer2(XMLNodeWriter::Options(), &stream2, log, 16);

                PRIME_TEST(writer2.writeDocument(nodeReader.getDocument(), true));

                log->output("%s\n", stream2.c_str());
            }
        }
    }
}

inline void XMLTests(Log* log)
{
    using namespace XMLTestsPrivate;

    XMLPullParserTest1(log);

    XMLPullParserTest2(log);

    XMLNodeTest(log);
}
}

#endif
