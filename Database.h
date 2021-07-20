// Copyright 2000-2021 Mark H. P. Lord

/*

    Implements a Python DB API style interface for accessing databases. Transactions are started implicitly for
    mutating SQL operations (INSERT/UPDATE/DELETE etc) and rolled back automatically if commit is not called.

    Field values are supplied and returned as Values.

    Example:

    RefPtr<Database> database = openDatabaseSomehow();

    RefPtr<DatabaseConnection> conn = database->connect(log);

    conn->createCursor(log)->execute("CREATE TABLE IF NOT EXISTS names ("
                                     "    id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                     "    name TEXT"
                                     ")", log);
    Value::Vector rows = {
        { "Beavis" },
        { "Butthead" },
        { "Björn" }
    };
    conn->createCursor(log)->executeMany("INSERT INTO names (name) VALUES (?)", log, rows);
    conn->commit(log);

    RefPtr<Cursor> cursor = conn->createCursor(log);
    if (cursor->execute("SELECT name, id FROM names WHERE names.name LIKE '?'", { "BJÖRN" }, log)) {
        while (cursor->fetch(log)) {
            printf("%3d: %s %d\n", (int) cursor->getRowNumber(), cursor->get("name").c_str(), cursor->get(1).toInt());
        }
        if (cursor->getError()) {
            ...
        }
    }

    cursor.release();
*/

#ifndef PRIME_DATABASE_H
#define PRIME_DATABASE_H

#include "ArrayView.h"
#include "Log.h"
#include "UnownedPtr.h"
#include "Value.h"

namespace Prime {

class Database;
class DatabaseConnection;

//
// SQLSyntax
//

class PRIME_PUBLIC SQLSyntax {
public:
    static const SQLSyntax* getSQLiteSyntax();
    static const SQLSyntax* getMySQLSyntax();

    // This determines how UTC datetimes are passed to MySQL
    bool isMySQL;

    // This determines whether our SQLite extensions are available
    bool isSQLite;

    // Can a primary key be an unsigned integer?
    bool allowUnsignedPrimaryKey;

    // AUTO_INCREMENT or AUTOINCREMENT?
    bool autoIncrementHasUnderscore;

    bool backslashesAlwaysEscape;

    // Is ESCAPE '\\' required?
    const char* wildcardToLeftModifier;

    // Is ALTER COLUMN available?
    bool canAlterColumn;

    // Is ALTER TABLE t RENAME COLUMN c TO n available?
    bool canAlterTableRenameColumn;

    // Is ALTER TABLE t DROP COLUMN c available?
    bool canAlterTableDropColumn;

    // true if we need to use CONCAT(a, b, c) to concatenate strings, otherwise a || b || c is used
    bool concatIsFunction;

    // Can "IF NOT EXISTS" be used when creating an index
    bool canCreateIndexIfNotExists;

    // Has "NOCASE" collation
    bool hasNocaseCollation;

    // Append a quoted string to an SQL statement
    void appendQuoted(std::string& target, StringView string, bool isWildcard) const;

    // Append a string to an SQL statement, escaping special characters and setting wildcardModifier to "ESCAPE '\\'"
    // where required
    void appendEscaped(std::string& target, StringView string, bool isWildcard, const char** wildcardModifier) const;

    std::string getEscaped(StringView string, bool isWildcard = false, const char** wildcardModifier = NULL) const;

    std::string getQuoted(StringView string, bool isWildcard = false) const;

    // Return "'%string%' WILDCARD_MODIFIER", so you can just append the return value to a LIKE.
    std::string getLikeExpression(StringView string) const;

private:
    SQLSyntax();
};

//
// ColumnNames
//

class PRIME_PUBLIC ColumnNames : public RefCounted {
public:
    ColumnNames();

    void clear();

    void reserve(size_t columnCount);

    void add(StringView columnName);

    bool empty() const { return _columns.empty(); }

    const size_t getColumnCount() const { return _columns.size(); }
    const std::string& getColumnName(size_t index) const { return _columns[index]; }

    /// Returns -1 if the column name isn't found
    ptrdiff_t find(StringView columnName) const;

private:
    std::vector<std::string> _columns;
};

//
// Rows
//

class Cursor;

/// Fetch rows from a Cursor with the ability to read all in advance or rewind.
class PRIME_PUBLIC Rows : public RefCounted {
public:
    explicit Rows(Cursor* cursor);

    ~Rows();

    /// Fetches one row from the cursor. Returns true on success, false on error or at the end of the results.
    /// Use getError() to determine whether an error occurred.
    bool fetch(Log* log);

    /// Fetches all the results. Returns true on success, even if the results are empty, and false on error.
    /// Use fetch() to then read the first result. Releases the cursor.
    bool fetchAll(Log* log);

    bool getError() const { return _errorFlag; }

    const Value& operator[](StringView columnName) const { return operator[](_columnNames.find(columnName)); }
    const Value& operator[](const char* columnName) const { return operator[](_columnNames.find(columnName)); }
    const Value& operator[](const std::string& columnName) const { return operator[](_columnNames.find(columnName)); }

    template <typename Index>
    const Value& operator[](Index index) const { return operator[](static_cast<size_t>(index)); }

    const Value& operator[](int index) const { return operator[](static_cast<size_t>(index)); }

    const Value& operator[](size_t index) const
    {
        if (index < _columnNames.getColumnCount() && _rowIndex >= 0 && !_atEnd) {
            PRIME_DEBUG_ASSERT(_rows[_rowIndex].size() == _columnNames.getColumnCount());
            return _rows[_rowIndex][index];
        }

        return Value::undefined;
    }

    Value::Dictionary getRowAsDictionary() const;

    const Value::Vector& getRowAsVector() const;

    void rewind()
    {
        _rowIndex = -1;
        _atEnd = false;
    }

private:
    RefPtr<Cursor> _cursor;
    ColumnNames _columnNames;
    std::vector<Value::Vector> _rows;
    ptrdiff_t _rowIndex;
    bool _errorFlag;
    bool _loaded;
    bool _atEnd;
};

//
// Cursor
//

/// A Cursor retains a reference to its DatabaseConnection, so the DatabaseConnection is automatically closed
/// when the last Cursor is destructed.
class PRIME_PUBLIC Cursor : public RefCounted {
public:
    Cursor();

    virtual ~Cursor() {};

    /// Specify whether or not to automatically begin/commit transactions.
    void setUseTransactions(bool value) { _useTransactions = value; }

    bool getUseTransactions() const { return _useTransactions; }

    /// On error, the cursor will still be closed and any error will be written to the Log.
    virtual bool close(Log* log) = 0;

    /// Begin a transaction.
    virtual bool begin(Log* log);

    bool execute(StringView sql, Log* log) { return executeOne(sql, ArrayView<const Value>(), log); }

    bool execute(StringView sql, ArrayView<const Value> bindings, Log* log)
    {
        return executeOne(sql, bindings, log);
    }

    virtual bool executeOne(StringView sql, ArrayView<const Value> bindings, Log* log) = 0;

    virtual bool executeMany(StringView sql, Log* log, const Value::Vector& rows);

    size_t getColumnCount() const { return _columnNames.getColumnCount(); }

    const std::string& getColumnName(size_t index) const { return _columnNames.getColumnName(index); }

    /// Returns false on error or if at the end of the results. Use getError() to determine if an error occurred.
    virtual bool fetch(Log* log) = 0;

    const Value& get(StringView columnName) const { return get(_columnNames.find(columnName)); }
    const Value& get(const char* columnName) const { return get(_columnNames.find(columnName)); }
    const Value& get(const std::string& columnName) const { return get(_columnNames.find(columnName)); }

    template <typename Index>
    const Value& get(Index index) const { return get(static_cast<size_t>(index)); }

    const Value& get(int index) const { return get(static_cast<size_t>(index)); }

    const Value& get(size_t index) const
    {
        if (index < _columnNames.getColumnCount() && !_errorFlag) {
            PRIME_DEBUG_ASSERT(_row.size() == _columnNames.getColumnCount());
            return _row[index];
        }

        return Value::undefined;
    }

    bool getError() const { return _errorFlag; }

    Value::Dictionary getRowAsDictionary() const;

    const Value::Vector& getRowAsVector() const { return _row; }

    void swapRow(Value::Vector& with);

    /// Returns a nullopt on error.
    Optional<std::vector<Value::Vector>> fetchAll(Log* log);

    /// Returns a nullopt on error, or a Value::Vector of Value::Dictionaries on success.
    Optional<Value::Vector> fetchAllAsDictionaries(Log* log);

    virtual size_t getRowNumber() const = 0;

    virtual size_t getChangeCount() const = 0;

    /// Returns the autoincrement primary key assigned to an insert.
    virtual int64_t getLastRowID() const = 0;

    virtual DatabaseConnection* getConnection() const = 0;

    Database* getDatabase() const;

    //
    // Helpers
    //

    /// Commit any current transaction on our connection.
    bool commit(Log* log);

    /// Roll-back any current transaction on our connection.
    bool rollback(Log* log);

    /// Create another Cursor on the same DatabaseConnection.
    RefPtr<Cursor> createCursor(Log* log);

    Value::Vector fetchFirstRow(StringView sql, ArrayView<const Value> bindings, Log* log);

    /// e.g., fetchFirstColumn("SELECT COUNT(*) FROM users", {}, log) -> undefined | int
    Value fetchFirstColumn(StringView sql, ArrayView<const Value> bindings, Log* log);

    /// e.g., fetchFirstColumn("SELECT COUNT(*) FROM users", log) -> undefined | int
    Value fetchFirstColumn(StringView sql, Log* log);

    /// Returns an undefined Value on end of results or on error. You must call either execute() or
    /// fetchFirstColumn(sql) first.
    Value fetchFirstColumn(Log* log);

protected:
    Cursor(bool useTransactions);

    ColumnNames _columnNames;
    Value::Vector _row;
    bool _errorFlag;

private:
    bool _useTransactions;
};

//
// BorrowCursor
//

/// If initialised with an existing cursor then simply acts as a wrapper around that cursor, otherwise constructs
/// a new cursor and commits it in commitUnlessBorrowed. This allows a function to optionally accept
/// a Cursor, and work within its transaction, or to create a new Cursor and commit when finished.
class BorrowCursor {
public:
    explicit BorrowCursor(Cursor* borrow, Database* database, Log* log);

    bool operator!() const { return !_cursor; }

    operator Cursor*() const { return _cursor; }

    Cursor* operator->() const { return _cursor; }

    Cursor* get() const { return _cursor; }

    /// Does nothing if we're borrowing a cursor and returns success, otherwise commits and returns
    /// the result.
    bool commitUnlessBorrowed(Log* log);

    bool isBorrowed() const { return !_ourCursor; }

private:
    UnownedPtr<Cursor> _cursor;
    RefPtr<Cursor> _ourCursor;
};

//
// DatabaseConnection
//

/// Provides a similar interface to databases as the Python DB API, composed of connections and cursors.
/// Connections cannot be shared across threads. SQL is executed by creating a cursor and calling its execute()
/// or executeMany() method, then retrieving results with the fetch() method.
/// Executing an INSERT, UPDATE, DELETE or REPLACE statement automatically begins a new transaction which can
/// be committed or rolled back using the commit() or rollback() methods of the connection object. commit() is
/// automatically called if executing an SQL statement other than INSERT, UPDATE, DELETE, REPLACE or SELECT.
/// If the connection is close()'d or released during a transaction, the transaction is rolled back.
class PRIME_PUBLIC DatabaseConnection : public RefCounted {
public:
    virtual ~DatabaseConnection() { }

    /// Close the connection now, rather than at destruction.
    virtual bool close(Log* log) = 0;

    /// Commit any pending transactions to the database. If you don't close a connection, pending transactions
    /// are rolled back.
    virtual bool commit(Log* log) = 0;

    /// Rollback any uncommited transaction. Not all database support this.
    virtual bool rollback(Log* log) = 0;

    struct CreateCursorOptions {
        CreateCursorOptions()
            : _automaticTransactions(true)
        {
        }

        CreateCursorOptions& disableAutomaticTransactions()
        {
            _automaticTransactions = false;
            return *this;
        }

        bool getAutomaticTransactions() const { return _automaticTransactions; }

    private:
        bool _automaticTransactions;
    };

    /// Create a new Cursor using this connection.
    virtual RefPtr<Cursor> createCursor(Log* log, const CreateCursorOptions& options = CreateCursorOptions()) = 0;

    virtual Database* getDatabase() = 0;

    virtual void* getHandle() = 0;

    virtual void appendQuoted(std::string& output, StringView string, bool isWildcard) const = 0;
    virtual void appendEscaped(std::string& output, StringView string, bool isWildcard, const char** wildcardModifier) const = 0;

    //
    // Utility methods
    //

    std::string getQuoted(StringView string, bool isWildcard) const;

    std::string getEscaped(StringView string, bool isWildcard, const char** wildcardModifier) const;

    // Return "'%string%' WILDCARD_MODIFIER", so you can just append the return value to a LIKE.
    std::string getLikeExpression(StringView string) const;
};

//
// Database
//

/// Represents a database, and provides connections.
class PRIME_PUBLIC Database : public RefCounted {
    PRIME_DECLARE_UID_CAST_BASE(0x6101590e, 0xe20f4718, 0x8c6d9cd2, 0xd9f39b45)

public:
    /// Implementers must derive their Options classes from this.
    class PRIME_PUBLIC Options {
    public:
        Options() { }

        virtual ~Options() { }
    };

    virtual ~Database() { }

    virtual RefPtr<DatabaseConnection> connect(Log* log) = 0;

    virtual const SQLSyntax* getSQLSyntax() const = 0;

    /// Shortcut for connect(log)->createCursor(log)
    RefPtr<Cursor> createCursor(Log* log,
        const DatabaseConnection::CreateCursorOptions& options = DatabaseConnection::CreateCursorOptions());

    //
    // Utility methods
    //

    enum StatementType {
        StatementTypeOther = 0,
        StatementTypeSelect,
        StatementTypeInsert,
        StatementTypeUpdate,
        StatementTypeDelete,
        StatementTypeReplace,
        StatementTypeCommit,
        StatementTypeRollback,
    };

    static StatementType detectStatementType(StringView sql);
};

//
// Cursor inlines
//

inline Database* Cursor::getDatabase() const
{
    return getConnection()->getDatabase();
}

inline bool Cursor::commit(Log* log)
{
    return getConnection()->commit(log);
}

inline bool Cursor::rollback(Log* log)
{
    return getConnection()->rollback(log);
}

inline RefPtr<Cursor> Cursor::createCursor(Log* log)
{
    return getConnection()->createCursor(log);
}
}

#endif
