// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CSVTESTS_H
#define PRIME_CSVTESTS_H

#include "CSVParser.h"
#include "CSVWriter.h"

namespace Prime {

namespace CSVTestsPrivate {

    inline void CSVEscapeTest()
    {
        char buffer[20];
        memset(buffer, '_', sizeof(buffer));
        static const char text[] = " Hello \"world\"! ";
        size_t ret = CSVWriter::escape(buffer, sizeof(buffer) - 1, StringView(text, sizeof(text) - 1)); // note sizeof(buffer) - 1 is for testing purposes
        PRIME_TESTMSG(ret == 20, "CSVWriter::escape computed wrong number of characters.");
        PRIME_TESTMSG(buffer[sizeof(buffer) - 1] == '_', "CSVWriter::escape overwrite the end of the buffer.");
        PRIME_TESTMSG(buffer[sizeof(buffer) - 2] == '\0', "CSVWriter::escape did not null terminate the buffer.");
        PRIME_TESTMSG(StringsEqual(buffer, "\" Hello \"\"world\"\"!"), "CSVWriter::escape output is incorrect.");
    }

    inline void TestCSVMemoryParser()
    {
        const char csv[] = "hello,this,is,the,first,line\r\n"
                           "\"this is the second line\",  \"it\"  , is  somewhat,\t\todd  \t\n"
                           "line 3\n\r"
                           "line 4";

        struct Expect {
            CSVParser::Token token;
            const char* text;
            const char* excelText;
        };

        const Expect expects[] = {
            { CSVParser::TokenText, "hello", 0 },
            { CSVParser::TokenText, "this", 0 },
            { CSVParser::TokenText, "is", 0 },
            { CSVParser::TokenText, "the", 0 },
            { CSVParser::TokenText, "first", 0 },
            { CSVParser::TokenText, "line", 0 },
            { CSVParser::TokenNewline, 0, 0 },
            { CSVParser::TokenText, "this is the second line", 0 },
            { CSVParser::TokenText, "it", "\"it\"" },
            { CSVParser::TokenText, "is  somewhat", 0 },
            { CSVParser::TokenText, "odd", 0 },
            { CSVParser::TokenNewline, 0, 0 },
            { CSVParser::TokenText, "line 3", 0 },
            { CSVParser::TokenNewline, 0, 0 },
            { CSVParser::TokenText, "line 4", 0 },
            { CSVParser::TokenEOF, 0, 0 },
        };

        for (int excelMode = 0; excelMode != 2; ++excelMode) {
            TextReader textReader;
            textReader.setText(csv);
            textReader.setLog(Log::getGlobal());

            CSVParser csvParser;
            csvParser.init(&textReader, CSVParser::Options().setExcelMode(excelMode == 1));

            for (const Expect* expect = expects; expect != expects + COUNTOF(expects); ++expect) {
                CSVParser::Token token = csvParser.read();

                if (token != expect->token) {
                    RuntimeError("Expected token %d, got token %d.", (int)expect->token, (int)token);
                    return;
                }

                const char* expectText;

                if (excelMode) {
                    expectText = expect->excelText;
                    if (!expectText) {
                        expectText = expect->text;
                    }
                } else {
                    expectText = expect->text;
                }

                if (expectText && !StringsEqual(csvParser.getText(), expectText)) {
                    RuntimeError("Expected: %s, got: %s", expectText, csvParser.getText().c_str());
                    return;
                }
            }
        }
    }
}

inline void CSVTests()
{
    using namespace CSVTestsPrivate;

    CSVEscapeTest();
    TestCSVMemoryParser();
}
}

#endif
