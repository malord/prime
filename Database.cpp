// Copyright 2000-2021 Mark H. P. Lord

#include "Database.h"
#include "StringUtils.h"

namespace Prime {

//
// SQLSyntax
//

SQLSyntax::SQLSyntax()
{
}

const SQLSyntax* SQLSyntax::getSQLiteSyntax()
{
    class SQLiteSyntax : public SQLSyntax {
    public:
        SQLiteSyntax()
        {
            isMySQL = false;
            isSQLite = true;
            allowUnsignedPrimaryKey = false;
            autoIncrementHasUnderscore = false;
            backslashesAlwaysEscape = false;
            wildcardToLeftModifier = " ESCAPE '\\' ";
            canAlterColumn = false;
            canAlterTableRenameColumn = true;
            canAlterTableDropColumn = false;
            concatIsFunction = false;
            canCreateIndexIfNotExists = true;
            hasNocaseCollation = true;
        }
    };

    static SQLiteSyntax sqlite;
    return &sqlite;
}

const SQLSyntax* SQLSyntax::getMySQLSyntax()
{
    class MySQLSyntax : public SQLSyntax {
    public:
        MySQLSyntax()
        {
            isMySQL = true;
            isSQLite = false;
            allowUnsignedPrimaryKey = true;
            autoIncrementHasUnderscore = true;
            backslashesAlwaysEscape = true;
            wildcardToLeftModifier = "";
            canAlterColumn = true;
            canAlterTableRenameColumn = false; // Not until v8!
            canAlterTableDropColumn = true;
            concatIsFunction = true;
            canCreateIndexIfNotExists = false;
            hasNocaseCollation = false;
        }
    };

    static MySQLSyntax mysql;
    return &mysql;
}

void SQLSyntax::appendQuoted(std::string& output, StringView string, bool isWildcard) const
{
    output += '\'';

    const char* wildcardModifier;

    appendEscaped(output, string, isWildcard, &wildcardModifier);

    output += '\'';

    if (wildcardModifier) {
        output += wildcardModifier;
    }
}

void SQLSyntax::appendEscaped(std::string& output, StringView string, bool isWildcard, const char** wildcardModifier) const
{
    const char escape = '\\';

    if (wildcardModifier) {
        *wildcardModifier = "";
    }

    const char* ptr = string.begin();
    const char* end = string.end();
    for (; ptr != end; ++ptr) {
        if (*ptr == '\'') {
            output += '\'';
        } else if (isWildcard) {
            if (*ptr == '%' || *ptr == '\'' || *ptr == escape) {
                if (PRIME_GUARD(wildcardModifier != NULL)) {
                    *wildcardModifier = this->wildcardToLeftModifier;
                }
                output += escape;
            }
        }

        output += *ptr;
    }
}

std::string SQLSyntax::getEscaped(StringView string, bool isWildcard, const char** wildcardModifier) const
{
    std::string output;
    appendEscaped(output, string, isWildcard, wildcardModifier);
    return output;
}

std::string SQLSyntax::getQuoted(StringView string, bool isWildcard) const
{
    std::string output;
    appendQuoted(output, string, isWildcard);
    return output;
}

std::string SQLSyntax::getLikeExpression(StringView string) const
{
    std::string output;
    const char* wildcardModifier;
    output += "'%";
    appendEscaped(output, string, true, &wildcardModifier);
    output += "%' ";
    output += wildcardModifier;
    return output;
}

//
// DatabaseConnection
//

std::string DatabaseConnection::getQuoted(StringView string, bool isWildcard) const
{
    std::string output;
    appendQuoted(output, string, isWildcard);
    return output;
}

std::string DatabaseConnection::getEscaped(StringView string, bool isWildcard, const char** wildcardModifier) const
{
    std::string output;
    appendEscaped(output, string, isWildcard, wildcardModifier);
    return output;
}

std::string DatabaseConnection::getLikeExpression(StringView string) const
{
    std::string output;
    const char* wildcardModifier;
    output += "'%";
    appendEscaped(output, string, true, &wildcardModifier);
    output += "%' ";
    output += wildcardModifier;
    return output;
}

//
// ColumnNames
//

ColumnNames::ColumnNames()
{
}

void ColumnNames::clear()
{
    _columns.clear();
}

void ColumnNames::reserve(size_t count)
{
    _columns.reserve(count);
}

void ColumnNames::add(StringView columnName)
{
    _columns.push_back(std::string(columnName.begin(), columnName.end()));
}

ptrdiff_t ColumnNames::find(StringView columnName) const
{
    // If this ever becomes a bottleneck, it can be optimised

    for (size_t i = 0; i != _columns.size(); ++i) {
        if (_columns[i] == columnName) {
            return static_cast<ptrdiff_t>(i);
        }
    }

    return -1;
}

//
// Rows
//

Rows::Rows(Cursor* cursor)
    : _cursor(cursor)
    , _rowIndex(-1)
    , _errorFlag(false)
    , _loaded(false)
    , _atEnd(false)
{
}

Rows::~Rows()
{
}

bool Rows::fetch(Log* log)
{
    if (_atEnd) {
        return false;
    }

    ++_rowIndex;
    if (_rowIndex < static_cast<ptrdiff_t>(_rows.size())) {
        return true;
    }

    PRIME_ASSERT(static_cast<size_t>(_rowIndex) <= _rows.size());

    if (_loaded) {
        _atEnd = true;
        return false;
    }

    if (_cursor->fetch(log)) {
        if (_columnNames.empty()) {
            _columnNames.reserve(_cursor->getColumnCount());
            for (size_t i = 0; i != _cursor->getColumnCount(); ++i) {
                _columnNames.add(_cursor->getColumnName(i));
            }
        }

        PRIME_DEBUG_ASSERT(static_cast<size_t>(_rowIndex) == _rows.size());
        _rows.push_back(Value::Vector());
        _cursor->swapRow(_rows.back());

        return true;
    }

    _loaded = true;
    if (_cursor->getError()) {
        _errorFlag = true;
        _rowIndex = -1;
    } else {
        _errorFlag = false;
    }
    _atEnd = true;
    _cursor.release();

    return false;
}

bool Rows::fetchAll(Log* log)
{
    if (_loaded) {
        return !_errorFlag;
    }
    _atEnd = false;

    ptrdiff_t nextWas = _rowIndex;
    while (fetch(log)) {
        // We're just here for the side effects
    }

    _cursor.release();

    PRIME_DEBUG_ASSERT(_loaded);

    if (_errorFlag) {
        return false;
    }

    _rowIndex = nextWas;
    _atEnd = nextWas >= 0 && static_cast<size_t>(nextWas) == _rows.size();
    return true;
}

Value::Dictionary Rows::getRowAsDictionary() const
{
    const Value::Vector& row = getRowAsVector();

    Value::Dictionary dict;

    if (!PRIME_GUARD(row.size() == _columnNames.getColumnCount())) {
        return dict;
    }

    dict.reserve(row.size());

    for (size_t i = 0; i != row.size(); ++i) {
        dict.set(_columnNames.getColumnName(i), row[i]);
    }

    return dict;
}

const Value::Vector& Rows::getRowAsVector() const
{
    if (_rowIndex < 0 || _atEnd) {
        return Value::emptyVector;
    }

    return _rows[_rowIndex];
}

//
// Cursor
//

Cursor::Cursor(bool useTransactions)
    : _useTransactions(useTransactions)
{
}

Cursor::Cursor()
    : _useTransactions(true)
{
}

bool Cursor::begin(Log* log)
{
    return execute("BEGIN", log);
}

bool Cursor::executeMany(StringView sql, Log* log, const Value::Vector& rows)
{
    for (size_t i = 0; i != rows.size(); ++i) {
        auto row = rows[i].toVector();
        if (!execute(sql, row, log)) {
            return false;
        }
    }

    return true;
}

Value::Dictionary Cursor::getRowAsDictionary() const
{
    if (!PRIME_GUARD(_row.size() == _columnNames.getColumnCount())) {
        return Value::Dictionary();
    }

    Value::Dictionary dict;
    dict.reserve(_columnNames.getColumnCount());

    for (size_t i = 0; i != _columnNames.getColumnCount(); ++i) {
        dict.set(_columnNames.getColumnName(i), _row[i]);
    }

    return dict;
}

void Cursor::swapRow(Value::Vector& with)
{
    _row.swap(with);
}

Value Cursor::fetchFirstColumn(Log* log)
{
    if (!fetch(log) || _row.empty()) {
        return undefined;
    }

    return _row[0];
}

Optional<std::vector<Value::Vector>> Cursor::fetchAll(Log* log)
{
    std::vector<Value::Vector> rows;
    while (fetch(log)) {
        rows.push_back(Value::Vector());
        _row.swap(rows.back());
    }

    if (getError()) {
        return nullopt;
    }

    return rows;
}

Optional<Value::Vector> Cursor::fetchAllAsDictionaries(Log* log)
{
    Value::Vector rows;
    while (fetch(log)) {
        rows.push_back(getRowAsDictionary());
    }

    if (getError()) {
        return nullopt;
    }

    return rows;
}

Value::Vector Cursor::fetchFirstRow(StringView sql, ArrayView<const Value> bindings, Log* log)
{
    Value::Vector result;
    if (execute(sql, bindings, log)) {
        if (fetch(log)) {
            swapRow(result);
        }
    }

    return result;
}

Value Cursor::fetchFirstColumn(StringView sql, ArrayView<const Value> bindings, Log* log)
{
    if (execute(sql, bindings, log)) {
        return fetchFirstColumn(log);
    }

    return undefined;
}

Value Cursor::fetchFirstColumn(StringView sql, Log* log)
{
    if (execute(sql, log)) {
        return fetchFirstColumn(log);
    }

    return undefined;
}

//
// BorrowCursor
//

BorrowCursor::BorrowCursor(Cursor* borrow, Database* database, Log* log)
{
    if (borrow) {
        _cursor = borrow;
    } else {
        _cursor = _ourCursor = database->createCursor(log);
    }
}

bool BorrowCursor::commitUnlessBorrowed(Log* log)
{
    return _ourCursor ? _ourCursor->commit(log) : true;
}

//
// Database
//

PRIME_DEFINE_UID_CAST_BASE(Database)

Database::StatementType Database::detectStatementType(StringView sql)
{
    int commentDepth = 0;

    for (;;) {
        sql = StringViewLeftTrim(sql);
        if (sql.empty()) {
            break;
        }

        if (!commentDepth && sql.size() >= 2 && sql[0] == '-' && sql[1] == '-') {
            sql = StringViewBisectLine(sql).second;

        } else if (!commentDepth && sql[0] == '#') { // # style comments for MySQL
            sql = StringViewBisectLine(sql).second;

        } else if (sql.size() >= 2 && sql[0] == '/' && sql[1] == '*') {
            ++commentDepth;

        } else if (commentDepth && sql.size() >= 2 && sql[0] == '*' && sql[1] == '/') {
            --commentDepth;

        } else if (!commentDepth) {
            break;
        }
    }

    char verb[10];

    char* out = verb;
    const char* ptr = sql.begin();
    while (ptr != sql.end() && ASCIIIsAlpha(*ptr) && out != &verb[sizeof(verb) - 2]) {
        *out++ = ASCIIToLower(*ptr++);
    }

    *out = 0;

    if (StringsEqual(verb, "select")) {
        return StatementTypeSelect;
    } else if (StringsEqual(verb, "insert")) {
        return StatementTypeInsert;
    } else if (StringsEqual(verb, "update")) {
        return StatementTypeUpdate;
    } else if (StringsEqual(verb, "delete")) {
        return StatementTypeDelete;
    } else if (StringsEqual(verb, "replace")) {
        return StatementTypeReplace;
    } else if (StringsEqual(verb, "commit")) {
        return StatementTypeCommit;
    } else if (StringsEqual(verb, "rollback")) {
        return StatementTypeRollback;
    }

    return StatementTypeOther;
}

RefPtr<Cursor> Database::createCursor(Log* log, const DatabaseConnection::CreateCursorOptions& options)
{
    RefPtr<DatabaseConnection> conn = connect(log);
    if (!conn) {
        return NULL;
    }

    return conn->createCursor(log, options);
}

}
