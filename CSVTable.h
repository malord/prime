// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CSVTABLE_H
#define PRIME_CSVTABLE_H

#include "ArrayView.h"
#include "CSVParser.h"
#include "Optional.h"
#include "UnownedPtr.h"
#include "Value.h"

namespace Prime {

/// Contains an entire table loaded in CSV format.
class PRIME_PUBLIC CSVTable {
public:
    CSVTable();

    ~CSVTable();

    bool load(Stream* stream, Log* log, const CSVParser::Options& options = CSVParser::Options());

    bool load(TextReader& textReader, const CSVParser::Options& options = CSVParser::Options());

    bool load(CSVParser& parser);

    size_t getRowCount() const { return _rows.size(); }

    class RowView {
    public:
        RowView(UnownedPtr<const CSVTable> table, size_t rowIndex, size_t columnCount)
            : _table(table)
            , _rowIndex(rowIndex)
            , _columnCount(columnCount)
        {
        }

        size_t getColumnCount() const { return _columnCount; }

        template <typename T>
        const std::string& operator[](const T& index) const { return _table->at(_rowIndex, index); }

        class const_iterator {
            UnownedPtr<const RowView> _rowView;
            size_t _columnIndex;

        public:
            const_iterator(const RowView* rowView, size_t columnIndex)
                : _rowView(rowView)
                , _columnIndex(columnIndex)
            {
            }

            bool operator==(const const_iterator& rhs) const
            {
                return _rowView == rhs._rowView && _columnIndex == rhs._columnIndex;
            }

            bool operator!=(const const_iterator& rhs) const
            {
                return !operator==(rhs);
            }

            bool operator<(const const_iterator& rhs) const
            {
                return _rowView == rhs._rowView && _columnIndex < rhs._columnIndex;
            }

            bool operator<=(const const_iterator& rhs) const
            {
                return _rowView == rhs._rowView && _columnIndex <= rhs._columnIndex;
            }

            bool operator>(const const_iterator& rhs) const
            {
                return _rowView == rhs._rowView && _columnIndex > rhs._columnIndex;
            }

            bool operator>=(const const_iterator& rhs) const
            {
                return _rowView == rhs._rowView && _columnIndex >= rhs._columnIndex;
            }

            const std::string& operator*() const
            {
                return _rowView->operator[](_columnIndex);
            }

            const_iterator operator++(int)
            {
                ++_columnIndex;
                return const_iterator(_rowView, _columnIndex - 1);
            }

            const_iterator& operator++()
            {
                ++_columnIndex;
                return *this;
            }
        };

        const_iterator begin() const { return const_iterator(this, 0); }
        const_iterator end() const { return const_iterator(this, _columnCount); }

        const_iterator cbegin() const { return const_iterator(this, 0); }
        const_iterator cend() const { return const_iterator(this, _columnCount); }

    private:
        UnownedPtr<const CSVTable> _table;
        size_t _rowIndex;
        size_t _columnCount;
    };

    const std::string& at(size_t rowIndex, StringView name) const { return _rows[rowIndex][lookup(name)]; }
    const std::string& at(size_t rowIndex, const char* name) const { return _rows[rowIndex][lookup(name)]; }
    const std::string& at(size_t rowIndex, const std::string& name) const { return _rows[rowIndex][lookup(name)]; }

    const std::string& at(size_t rowIndex, size_t columnIndex) const { return _rows[rowIndex][columnIndex]; }
    const std::string& at(size_t rowIndex, Optional<size_t> columnIndex) const { return _rows[rowIndex][columnIndex.value_or(SIZE_MAX)]; }

    RowView operator[](size_t rowIndex) const { return RowView(this, rowIndex, _rows[rowIndex].size()); }

    Optional<size_t> findColumn(StringView name) const;

    bool findHeaderRow(ArrayView<const char*> fieldNames);

    /// This is automatically called with rowIndex == 0, but you can call it if you knoew there's a preamble
    void setHeaderRowIndex(size_t rowIndex);

    size_t getHeaderRowIndex() const { return _headerRowIndex; }

    Value::Dictionary getRowAsDictionary(size_t rowIndex) const;

private:
    class Row;

    bool loadRow(CSVParser& parser, Row& row, bool& more);

    size_t lookup(StringView name) const;

    class Row {
    public:
        void addColumn(StringView value);

        bool empty() const { return _columns.empty(); }

        size_t size() const { return _columns.size(); }

        void reserve(size_t capacity) { _columns.reserve(capacity); }

        void clear() { _columns.clear(); }

        const std::string& operator[](size_t index) const
        {
            if (index >= _columns.size()) {
                return badColumn;
            }

            return _columns[index];
        }

    private:
        std::vector<std::string> _columns;

        static const std::string badColumn;
    };

    Row _headerRow;
    size_t _headerRowIndex;

    std::vector<Row> _rows;
};
}

#endif
