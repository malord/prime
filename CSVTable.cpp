// Copyright 2000-2021 Mark H. P. Lord

#include "CSVTable.h"
#include "StringUtils.h"

namespace Prime {

//
// CSVTable::Row
//

const std::string CSVTable::Row::badColumn("");

void CSVTable::Row::addColumn(StringView value)
{
    _columns.push_back(std::string(value.begin(), value.end()));
}

//
// CSVTable
//

CSVTable::CSVTable()
{
    _headerRowIndex = 0;
}

CSVTable::~CSVTable()
{
}

bool CSVTable::load(Stream* stream, Log* log, const CSVParser::Options& options)
{
    TextReader textReader;
    textReader.setLog(log);
    textReader.setStream(stream, TextReader::defaultBufferSize);
    return load(textReader, options);
}

bool CSVTable::load(TextReader& textReader, const CSVParser::Options& options)
{
    CSVParser parser;
    parser.init(&textReader, options);

    return load(parser);
}

bool CSVTable::load(CSVParser& parser)
{
    PRIME_ASSERT(_headerRow.empty());

    bool more = false;
    do {
        _rows.push_back(Row());
        if (!_rows.empty()) {
            _rows.back().reserve(_rows.front().size());
        }
        if (!loadRow(parser, _rows.back(), more)) {
            return false;
        }
    } while (more);

    if (!_rows.empty()) {
        // This is useful in some cases, but not the general case
        // bool reportedShortRow = false;
        // for (size_t i = 1; i != _rows.size(); ++i) {
        //     if (_rows[i].empty()) {
        //         if (i != _rows.size() - 1) {
        //             parser.getLog()->warning("Row %" PRIuPTR " empty", i);
        //         }

        //         _rows.erase(_rows.begin() + i);
        //         --i;
        //         continue;
        //     }

        //     if (_rows[i].size() < _rows[0].size()) {
        //         if (! reportedShortRow) {
        //             parser.getLog()->warning("At least one row (%" PRIuPTR ") shorter than first row", i);
        //             reportedShortRow = true;
        //         }
        //         while (_rows[i].size() < _rows[0].size()) {
        //             _rows[i].addColumn(StringView());
        //         }

        //         continue;
        //     }

        //     if (_rows[i].size() > _rows[0].size()) {
        //         parser.getLog()->warning("Row %" PRIuPTR " longer than first row", i);
        //         continue;
        //     }
        // }

        setHeaderRowIndex(0);
    }

    return true;
}

bool CSVTable::findHeaderRow(ArrayView<const char*> fieldNames)
{
    for (size_t rowIndex = 0; rowIndex != getRowCount(); ++rowIndex) {
        auto row = operator[](rowIndex);

        if (row.getColumnCount() >= fieldNames.size()) {
            bool matched = true;
            for (size_t columnIndex = 0; columnIndex != fieldNames.size(); ++columnIndex) {
                if (!StringsEqual(fieldNames[columnIndex], StringViewTrim(row[columnIndex]))) {
                    matched = false;
                    break;
                }
            }

            if (matched) {
                setHeaderRowIndex(rowIndex);
                return true;
            }
        }
    }

    return false;
}

void CSVTable::setHeaderRowIndex(size_t rowIndex)
{
    if (!PRIME_GUARD(rowIndex < _rows.size())) {
        return;
    }

    _headerRowIndex = rowIndex;
    _headerRow.clear();

    for (size_t i = 0; i != _rows[rowIndex].size(); ++i) {
        _headerRow.addColumn(StringTrim(_rows[rowIndex][i]));
    }
}

Optional<size_t> CSVTable::findColumn(StringView name) const
{
    for (size_t i = 0; i != _headerRow.size(); ++i) {
        if (ASCIIEqualIgnoringCase(name, _headerRow[i])) {
            return i;
        }
    }

    return nullopt;
}

size_t CSVTable::lookup(StringView name) const
{
    return findColumn(name).value_or(SIZE_MAX);
}

bool CSVTable::loadRow(CSVParser& parser, Row& row, bool& more)
{
    more = false;
    for (;;) {
        CSVParser::Token token = parser.read();
        if (token == CSVParser::TokenError) {
            return false;
        }
        if (token == CSVParser::TokenEOF) {
            return true;
        }
        if (token == CSVParser::TokenNewline) {
            more = true;
            return true;
        }

        row.addColumn(parser.getText());
    }
}

Value::Dictionary CSVTable::getRowAsDictionary(size_t rowIndex) const
{
    RowView row(this, rowIndex, _rows[rowIndex].size());

    Value::Dictionary dict;
    for (size_t i = 0; i != row.getColumnCount(); ++i) {
        if (i < _headerRow.size()) {
            dict.set(_headerRow[i], row[i]);
        }
    }

    return dict;
}
}
