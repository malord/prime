// Copyright 2000-2021 Mark H. P. Lord

#include "MySQLDatabase.h"

#ifdef PRIME_HAVE_MYSQLDATABASE

#include "JSONWriter.h"
#include "NumberUtils.h"
#include "PrefixLog.h"
#include "StringUtils.h"
#include <mysql.h>

namespace Prime {

//
// MySQLDatabase::MySQLConnection declaration
//

class MySQLDatabase::MySQLConnection : public DatabaseConnection {
public:
    explicit MySQLConnection(MySQLDatabase* db);

    virtual ~MySQLConnection();

    bool isConnected() const { return _connected; }

    bool connect(Log* log);

    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual bool commit(Log* log) PRIME_OVERRIDE;
    virtual bool rollback(Log* log) PRIME_OVERRIDE;
    virtual RefPtr<Cursor> createCursor(Log* log, const CreateCursorOptions& options) PRIME_OVERRIDE;
    virtual Database* getDatabase() PRIME_OVERRIDE;
    virtual void* getHandle() PRIME_OVERRIDE;
    virtual void appendQuoted(std::string& output, StringView string, bool isWildcard) const PRIME_OVERRIDE;
    virtual void appendEscaped(std::string& output, StringView string, bool isWildcard, const char** wildcardModifier) const PRIME_OVERRIDE;

private:
    friend class MySQLCursor;

    bool needTransaction(Log* log);
    bool endTransacton(const char* statement, Log* log);

    void logError(Log* log, const char* cause);

    mutable MYSQL _mysql;

    RefPtr<MySQLDatabase> _db;

    bool _connected;
    bool _inTransaction;
    bool _verboseLogging;

    PRIME_UNCOPYABLE(MySQLConnection);
};

//
// MySQLDatabase::MySQLCursor declaration
//

class MySQLDatabase::MySQLCursor : public Cursor {
public:
    MySQLCursor(MySQLConnection* connection, bool useTransactions);

    virtual ~MySQLCursor();

    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual bool executeOne(StringView sql, ArrayView<const Value> bindings, Log* log) PRIME_OVERRIDE;
    virtual bool fetch(Log* log) PRIME_OVERRIDE;
    virtual size_t getRowNumber() const PRIME_OVERRIDE;
    virtual size_t getChangeCount() const PRIME_OVERRIDE;
    virtual int64_t getLastRowID() const PRIME_OVERRIDE;
    virtual DatabaseConnection* getConnection() const PRIME_OVERRIDE;

private:
    RefPtr<MySQLConnection> _connection;

    StatementType _statementType;

    MYSQL_RES* _res;
    int64_t _lastRowID;
    size_t _changeCount;
    size_t _rowNumber;

    PRIME_UNCOPYABLE(MySQLCursor);
};

//
// MySQLDatabase::MySQLConnection implementation
//

MySQLDatabase::MySQLConnection::MySQLConnection(MySQLDatabase* db)
    : _db(db)
    , _connected(false)
    , _inTransaction(false)
{
}

MySQLDatabase::MySQLConnection::~MySQLConnection()
{
    close(Log::getGlobal());
}

void MySQLDatabase::MySQLConnection::logError(Log* log, const char* cause)
{
    PRIME_ASSERT(isConnected());
    log->runtimeError(PRIME_LOCALISE("%s: %s"), cause, mysql_error(&_mysql));
}

bool MySQLDatabase::MySQLConnection::connect(Log* log)
{
    PRIME_ASSERT(!isConnected());
    PRIME_ASSERT(GlobalInitialisation::getSingleton()->isInitialised());

    _verboseLogging = _db->getOptions().getVerboseLogging();

    if (!mysql_init(&_mysql)) {
        log->error(PRIME_LOCALISE("mysql_init failed"));
        return false;
    }

    if (!mysql_real_connect(&_mysql, _db->getHost(), _db->getUser(), _db->getPassword(),
            _db->getDatabaseName(), _db->getPort(), _db->getUnixSocketOrNull(),
            _db->getClientFlags())) {
        log->error(PRIME_LOCALISE("Failed to connect to database: %s"), mysql_error(&_mysql));
        mysql_close(&_mysql);
        return false;
    }

    _connected = true;
    _inTransaction = false;

    return true;
}

bool MySQLDatabase::MySQLConnection::close(Log* log)
{
    if (!_connected) {
        return true;
    }

    bool success = true;

    if (_inTransaction) {
        if (!rollback(log)) {
            success = false;
        }
    }

    mysql_close(&_mysql);
    mysql_thread_end(); // According to the internet, this is necessary to avoid a memory leak
    _connected = false;

    return success;
}

bool MySQLDatabase::MySQLConnection::needTransaction(Log* log)
{
    PRIME_ASSERT(isConnected());

    if (_inTransaction) {
        return true;
    }

    //log->trace("SQLite: BEGIN transaction.");

    MySQLCursor cursor(this, false);
    if (!cursor.execute("BEGIN", log) || !cursor.close(log)) {
        return false;
    }

    _inTransaction = true;
    return true;
}

bool MySQLDatabase::MySQLConnection::commit(Log* log)
{
    return endTransacton("COMMIT", log);
}

bool MySQLDatabase::MySQLConnection::rollback(Log* log)
{
    return endTransacton("ROLLBACK", log);
}

bool MySQLDatabase::MySQLConnection::endTransacton(const char* statement, Log* log)
{
    PRIME_ASSERT(isConnected());

    if (!_inTransaction) {
        return true;
    }

    MySQLCursor cursor(this, false);
    bool success = cursor.execute(statement, log) && cursor.close(log);
    _inTransaction = false;
    return success;
}

RefPtr<Cursor> MySQLDatabase::MySQLConnection::createCursor(Log*, const CreateCursorOptions& options)
{
    PRIME_ASSERT(isConnected());

    RefPtr<MySQLCursor> cursor = PassRef(new MySQLCursor(this, options.getAutomaticTransactions()));

    return cursor;
}

Database* MySQLDatabase::MySQLConnection::getDatabase()
{
    return _db;
}

void* MySQLDatabase::MySQLConnection::getHandle()
{
    return &_mysql;
}

void MySQLDatabase::MySQLConnection::appendQuoted(std::string& output, StringView string, bool isWildcard) const
{
    if (!PRIME_GUARD(isConnected())) {
        return;
    }

    output += '\'';
    appendEscaped(output, string, isWildcard, NULL);
    output += '\'';
}

void MySQLDatabase::MySQLConnection::appendEscaped(std::string& output, StringView string, bool isWildcard,
    const char** wildcardModifier) const
{
    if (!PRIME_GUARD(isConnected())) {
        return;
    }

    if (wildcardModifier) {
        *wildcardModifier = "";
    }

    if (isWildcard) {
        SQLSyntax::getMySQLSyntax()->appendEscaped(output, string, isWildcard, wildcardModifier);
    } else {
        size_t sizeWas = output.size();
        output.resize(sizeWas + string.size() * 2 + 1);
        unsigned long newLength = mysql_real_escape_string(&_mysql, &output[sizeWas], string.data(), Narrow<unsigned long>(string.size()));
        output.resize((size_t)newLength + sizeWas);
    }
}

//
// MySQLDatabase::MySQLCursor implementation
//

MySQLDatabase::MySQLCursor::MySQLCursor(MySQLConnection* connection, bool useTransactions)
    : Cursor(useTransactions)
    , _connection(connection)
    , _res(NULL)
{
}

MySQLDatabase::MySQLCursor::~MySQLCursor()
{
    close(Log::getGlobal());
}

bool MySQLDatabase::MySQLCursor::close(Log* log)
{
    (void)log;
    if (_res) {
        mysql_free_result(_res);
        _res = NULL;
    }

    return true;
}

bool MySQLDatabase::MySQLCursor::executeOne(StringView sql, ArrayView<const Value> bindings, Log* log)
{
    close(PrefixLog(log, "Closing previous cursor"));

    // Some duplication of effort here, detectStatementType is scanning the SQL
    if (getUseTransactions()) {
        _statementType = detectStatementType(sql);

        switch (_statementType) {
        case StatementTypeInsert:
        case StatementTypeUpdate:
        case StatementTypeDelete:
        case StatementTypeReplace:
            if (!_connection->needTransaction(log)) {
                return false;
            }
            break;

        case StatementTypeOther:
            _connection->commit(log);
            break;

        case StatementTypeCommit:
        case StatementTypeRollback:
        case StatementTypeSelect:
            break;

        default:
            PRIME_ASSERT(0);
        }
    }

    std::string encoded;
    encoded.reserve(1024);

    // The MySQL API for prepared statements isn't pretty and probably too strict for this API, where a date can
    // sometimes be a string and sometimes be a UnixTime, so just encode everything as strings.
    // Takes SQL, which contains ? for bindings, and copy it to encoded with the ?s replaced by the string-encoded
    // bindings.

    const char* sqlPtr = sql.begin();
    const char* sqlEnd = sql.end();

    size_t bindingIndex = 0;

    while (sqlPtr != sqlEnd) {
        const char* begin = sqlPtr;
        while (sqlPtr != sqlEnd && !strchr("\"'-/;#?", *sqlPtr)) {
            ++sqlPtr;
        }

        encoded.append(begin, sqlPtr);

        if (sqlPtr == sqlEnd) {
            break;
        }

        const char* begin2 = sqlPtr;
        bool handled = false;

        switch (*sqlPtr) {
        case '"': // A string containing escape sequences
        case '\'': {
            ++sqlPtr;

            for (; sqlPtr != sqlEnd; ++sqlPtr) {
                if (*sqlPtr == *begin2) {
                    if (sqlPtr + 1 != sqlEnd && sqlPtr[1] == *begin2) {
                        // It's escaped
                        ++sqlPtr;
                    } else {
                        ++sqlPtr;
                        break;
                    }
                } else if (*sqlPtr == '\\') {
                    if (sqlPtr + 1 != sqlEnd) {
                        ++sqlPtr;
                    }
                }
            }
            break;
        }

        case '-': // "-- " (must be followed by one whitespace character)
            if (sqlEnd - sqlPtr >= 3 && sqlPtr[1] == '-' && ASCIIIsWhitespace(sqlPtr[2])) {
                // Single line comment.
                sqlPtr = ASCIISkipNewline(sqlPtr + 3, sqlEnd);
            }
            break;

        case '#': // Comment to the end of the line
            sqlPtr = ASCIISkipNewline(sqlPtr + 1, sqlEnd);
            break;

        case '/': // Check for C style comments
            if (sqlPtr + 1 != sqlEnd && sqlPtr[1] == '*') {
                int depth = 1;
                sqlPtr += 2;
                for (; sqlPtr != sqlEnd; ++sqlPtr) {
                    if (sqlPtr[0] == '/') {
                        if (sqlPtr + 1 != sqlEnd && sqlPtr[1] == '*') {
                            ++depth;
                            ++sqlPtr;
                        }
                    } else if (sqlPtr[0] == '*') {
                        if (sqlPtr + 1 != sqlEnd && sqlPtr[1] == '/') {
                            ++sqlPtr;
                            if (--depth == 0) {
                                ++sqlPtr;
                                break;
                            }
                        }
                    }
                }
            }
            break;

        case ';': // Semicolon just terminates the expression
            sqlEnd = sqlPtr + 1;
            break;

        case '?': { // Yay, a binding!
            ++sqlPtr;
            begin2 = sqlPtr;
            handled = true;

            if (bindingIndex == bindings.size()) {
                log->error(PRIME_LOCALISE("MySQL: insufficient bindings."));
                return false;
            }

            const Value& binding = bindings[bindingIndex++];
            switch (binding.getType()) {
            case Value::TypeUndefined:
                log->error(PRIME_LOCALISE("MySQL: binding is undefined."));
                return false;

            case Value::TypeNull:
                encoded += "NULL";
                break;

            case Value::TypeBool:
                encoded += binding.getBool() ? "1" : "0";
                break;

            case Value::TypeInteger:
                StringAppendFormat(encoded, "%" PRIME_PRId_VALUE, binding.getInteger());
                break;

            case Value::TypeReal:
                StringAppendFormat(encoded, "%" PRIME_PRIg_VALUE, binding.getReal());
                break;

            case Value::TypeString:
                _connection->appendQuoted(encoded, binding.getString(), false);
                break;

            case Value::TypeData:
                _connection->appendQuoted(encoded, StringView((const char*)binding.getData().data(), binding.getData().size()), false);
                break;

            case Value::TypeDate:
                encoded += '\'';
                encoded += binding.toString(); // ISO-8601
                encoded += '\'';
                break;

            case Value::TypeTime:
                encoded += '\'';
                encoded += binding.toString(); // 24 hour
                encoded += '\'';
                break;

            case Value::TypeDateTime:
                encoded += '\'';
                encoded += binding.toString(); // ISO-8601
                encoded += '\'';
                break;

            case Value::TypeVector:
            case Value::TypeDictionary:
                _connection->appendQuoted(encoded, ToJSON(binding), false);
                break;

            case Value::TypeObject:
                log->error(PRIME_LOCALISE("MySQL: binding is an object."));
                return false;
            }
            break;
        }

        default:
            RuntimeError("strchr and switch out of sync");
            break;
        }

        if (!handled) {
            if (sqlPtr == begin2) {
                // Wasn't handled. Skip it.
                encoded += *sqlPtr++;
            } else {
                encoded.append(begin2, sqlPtr);
            }
        }
    }

    if (_connection->_verboseLogging) {
        log->trace("MySQL: %s", encoded.c_str());
    }

    if (mysql_real_query(&_connection->_mysql, encoded.c_str(), Narrow<unsigned long>(encoded.size())) != 0) {
        _connection->logError(log, "mysql_real_query");
        _errorFlag = true;
        return false;
    }

    _columnNames.clear();
    _rowNumber = (size_t)-1;

    if (mysql_field_count(&_connection->_mysql) != 0) {
        _res = mysql_store_result(&_connection->_mysql);

        if (_res) {
            while (MYSQL_FIELD* field = mysql_fetch_field(_res)) {
                _columnNames.add(field->name);
            }
        }
    } else {
        if (!PRIME_GUARD(_res == NULL)) {
            mysql_free_result(_res);
            _res = NULL;
        }
    }

    _changeCount = Narrow<size_t>(mysql_affected_rows(&_connection->_mysql));

    if (_statementType == StatementTypeInsert) {
        _lastRowID = (int64_t)mysql_insert_id(&_connection->_mysql);
    } else {
        _lastRowID = -1;
    }

    return true;
}

bool MySQLDatabase::MySQLCursor::fetch(Log* log)
{
    _errorFlag = false;
    if (_res) {
        if (MYSQL_ROW strings = mysql_fetch_row(_res)) {
            ++_rowNumber;

            unsigned long* lengths = mysql_fetch_lengths(_res);
            size_t columnCount = _columnNames.getColumnCount();

            _row.resize(columnCount);
            for (size_t i = 0; i != columnCount; ++i) {
                if (strings[i]) {
                    _row[i] = std::string(strings[i], Narrow<size_t>(lengths[i]));
                } else {
                    _row[i] = null;
                }
            }

            return true;
        }

        if (mysql_errno(&_connection->_mysql)) {
            _connection->logError(log, "mysql_fetch_row");
            _row.clear();
            _errorFlag = true;
            return false;
        }
    }

    _row.clear();
    return false;
}

size_t MySQLDatabase::MySQLCursor::getRowNumber() const
{
    return _rowNumber;
}

size_t MySQLDatabase::MySQLCursor::getChangeCount() const
{
    return _changeCount;
}

int64_t MySQLDatabase::MySQLCursor::getLastRowID() const
{
    return _lastRowID;
}

DatabaseConnection* MySQLDatabase::MySQLCursor::getConnection() const
{
    return _connection;
}

//
// MySQLDatabase::GlobalInitialisation implementation
//

MySQLDatabase::GlobalInitialisation* MySQLDatabase::GlobalInitialisation::getSingleton()
{
    static GlobalInitialisation singleton;
    return &singleton;
}

MySQLDatabase::GlobalInitialisation::GlobalInitialisation()
    : _initialised(0)
{
    init();
}

MySQLDatabase::GlobalInitialisation::~GlobalInitialisation()
{
    shutdown();
}

void MySQLDatabase::GlobalInitialisation::init()
{
    if (_initialised++) {
        return;
    }

    if (mysql_library_init(0, NULL, NULL)) {
        RuntimeError(PRIME_LOCALISE("Could not initialise MySQL library."));
    }
}

void MySQLDatabase::GlobalInitialisation::shutdown()
{
    PRIME_ASSERT(_initialised > 0);
    if (!--_initialised) {
        return;
    }

    mysql_library_end();
}

//
// MySQLDatabase
//

PRIME_DEFINE_UID_CAST(MySQLDatabase)

MySQLDatabase::MySQLDatabase()
{
    globalInit();
}

MySQLDatabase::~MySQLDatabase()
{
}

bool MySQLDatabase::init(const Options& options, Log* log)
{
    PRIME_ALWAYS_ASSERTMSG(mysql_thread_safe(), "MariaDB connector library compiled non-thread-safe");

    log->trace("MySQL client version: %s", mysql_get_client_info());

    _options = options;

    return true;
}

RefPtr<DatabaseConnection> MySQLDatabase::connect(Log* log)
{
    RefPtr<MySQLConnection> conn = PassRef(new MySQLConnection(this));

    if (!conn->connect(log)) {
        return NULL;
    }

    return conn;
}

const SQLSyntax* MySQLDatabase::getSQLSyntax() const
{
    return SQLSyntax::getMySQLSyntax();
}
}

#endif // PRIME_HAVE_MYSQLDATABASE
