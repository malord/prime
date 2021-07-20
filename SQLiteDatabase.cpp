// Copyright 2000-2021 Mark H. P. Lord

#include "SQLiteDatabase.h"

#ifdef PRIME_HAVE_SQLITEDATABASE

#include "Clocks.h"
#include "Decimal.h"
#include "JSONWriter.h"
#include "NumberUtils.h"
#include "PrefixLog.h"
#include "StringUtils.h"
#include "Templates.h"
#include "sqlite3.h"
#include "sqlite3_unicode.h"

// TODO: put the extension functions in a .so so the sqlite3 tool can load them
// (see http://blogs.operationaldynamics.com/andrew/software/objective/defining-sqlite-functions)

static const unsigned int millisecondsToSleepBeforeRetry = 50;

namespace Prime {

//
// SQLiteDatabase::SQLiteConnection declaration
//

class SQLiteDatabase::SQLiteConnection : public DatabaseConnection {
public:
    void logSQLiteError(Log* log, int error, StringView cause);

    SQLiteConnection();

    ~SQLiteConnection();

    bool open(Database* database, const char* path, const Options& options, Log* log);

    bool isOpen() const { return _db != NULL; }

    sqlite3* getSQLite3() const { return _db; }

    bool isVerboseLoggingEnabled() const { return _verboseLogging; }

    // DatabaseConnection implementation.
    virtual RefPtr<Cursor> createCursor(Log* log, const CreateCursorOptions& options) PRIME_OVERRIDE;
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual bool commit(Log* log) PRIME_OVERRIDE;
    virtual bool rollback(Log* log) PRIME_OVERRIDE;
    virtual Database* getDatabase() PRIME_OVERRIDE { return _database; }
    virtual void* getHandle() PRIME_OVERRIDE { return getSQLite3(); }
    virtual void appendQuoted(std::string& output, StringView string, bool isWildcard) const PRIME_OVERRIDE;
    virtual void appendEscaped(std::string& output, StringView string, bool isWildcard, const char** wildcardModifier) const PRIME_OVERRIDE;

protected:
    friend class SQLiteCursor;

    bool needTransaction(Log* log);

    bool inTransaction();

private:
    bool endTransaction(const char* statement, Log* log);

    sqlite3* _db;
    bool _verboseLogging;
    RefPtr<Database> _database;

    PRIME_UNCOPYABLE(SQLiteConnection);
};

static int IntegrityCheckCallback(void* context, int argc, char** argv, char** column)
{
    PRIME_UNUSED(column);
    PRIME_UNUSED(context);
    Log* log = reinterpret_cast<Log*>(context);
    for (int i = 0; i < argc; i++) {
        log->warning("%s", argv[i]);
    }
    return 0;
}

//
// SQLiteDatabase::SQLiteCursor declaration
//

class SQLiteDatabase::SQLiteCursor : public Cursor {
public:
    ~SQLiteCursor();

    bool isPrepared() const { return _statement != NULL; }

    bool reset(Log* log);

    // Cursor implementation.
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual bool begin(Log* log) PRIME_OVERRIDE;
    virtual bool executeOne(StringView sql, ArrayView<const Value> bindings, Log* log) PRIME_OVERRIDE;
    virtual bool fetch(Log* log) PRIME_OVERRIDE;
    virtual size_t getRowNumber() const PRIME_OVERRIDE { return _rowNumber; }
    virtual size_t getChangeCount() const PRIME_OVERRIDE { return _changeCount; }
    virtual int64_t getLastRowID() const PRIME_OVERRIDE { return _lastRowID; }
    virtual DatabaseConnection* getConnection() const PRIME_OVERRIDE { return _connection; }

protected:
    friend class SQLiteConnection;
    friend class SQLiteDatabase;

    explicit SQLiteCursor(SQLiteConnection* connection, bool useTransactions);

private:
    /// Bind to a zero-based parameter of the prepared statement. Returns SQLite error code.
    int sqliteBind(int index, const Value& value, Log* log);

    bool step(Log* log, StringView sql);

    RefPtr<SQLiteConnection> _connection;
    sqlite3_stmt* _statement;
    StatementType _statementType;

    size_t _rowNumber;
    size_t _changeCount;
    int _stepState;
    int64_t _lastRowID;

    PRIME_UNCOPYABLE(SQLiteCursor);
};

//
// SQLiteDatabase::SQLiteCursor implementation
//

SQLiteDatabase::SQLiteCursor::SQLiteCursor(SQLiteConnection* connection, bool useTransactions)
    : Cursor(useTransactions)
    , _connection(connection)
    , _statement(NULL)
{
}

SQLiteDatabase::SQLiteCursor::~SQLiteCursor()
{
    close(Log::getGlobal());
}

bool SQLiteDatabase::SQLiteCursor::close(Log* log)
{
    if (!_statement) {
        return true;
    }

    int result = sqlite3_finalize(_statement);
    _statement = NULL;

    if (result != SQLITE_OK) {
        _connection->logSQLiteError(log, result, "sqlite3_finalize");
        return false;
    }

    return true;
}

bool SQLiteDatabase::SQLiteCursor::begin(Log* log)
{
    return execute("BEGIN IMMEDIATE TRANSACTION", log);
}

bool SQLiteDatabase::SQLiteCursor::executeOne(StringView sql, ArrayView<const Value> bindings, Log* log)
{
    close(PrefixLog(log, "Closing previous cursor"));
    _columnNames.clear();

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

    if (_connection->isVerboseLoggingEnabled()) {
        if (bindings.empty()) {
            log->trace(MakeString("SQLite: \"", sql, "\""));
        } else {
            log->trace(MakeString("SQLite: \"", sql, "\" [", bindings, ']'));
        }
    }

    const char* tail = NULL;
    int result = sqlite3_prepare_v2(_connection->getSQLite3(), sql.data(), Narrow<int>(sql.size()), &_statement, &tail);
    if (result != SQLITE_OK) {
        _statement = NULL;
        _connection->logSQLiteError(log, result, StringView("sqlite3_prepare_v2: ") + sql);
        return false;
    }

    _rowNumber = (size_t)-1;
    _stepState = SQLITE_OK;
    _changeCount = (size_t)-1;
    _lastRowID = -1;
    _columnNames.clear();

    // Could cache number of columns and their names.

    for (unsigned int index = 0; index != bindings.size(); ++index) {
        int bindResult = sqliteBind((int)index, bindings[index], log);
        if (bindResult == SQLITE_RANGE) {
            // Probably still got some binding set from a previous statement.
            break;
        }
        if (bindResult != SQLITE_OK) {
            _connection->logSQLiteError(log, bindResult, StringView("sqlite3_bind: ") + sql);
            return false;
        }
    }

    return step(log, sql);
}

bool SQLiteDatabase::SQLiteCursor::step(Log* log, StringView sql)
{
    (void)log;

    if (_stepState == SQLITE_DONE) {
        return true;
    }

    for (;;) {
        _stepState = sqlite3_step(_statement);

        if (_stepState == SQLITE_ROW) {
            ++_rowNumber;
            return true;
        }

        if (_stepState == SQLITE_DONE) {
            _changeCount = (size_t)sqlite3_changes(_connection->getSQLite3());

            if (_statementType == StatementTypeInsert) {
                _lastRowID = sqlite3_last_insert_rowid(_connection->getSQLite3());
            }

            return true;
        }

        if (_stepState == SQLITE_BUSY) {
            log->trace("SQLITE_BUSY");

            // "If the statement is a COMMIT or occurs outside of an explicit transaction, then you can retry the statement"
            if (_statementType == StatementTypeCommit || !_connection->inTransaction()) {
                // We can retry.
                Clock::sleepMilliseconds(millisecondsToSleepBeforeRetry);
                continue;
            }

            // "If the statement is not a COMMIT and occurs within an explicit transaction then you should rollback the transaction before continuing"
        }

        PRIME_ASSERT(_stepState != SQLITE_MISUSE);

        _connection->logSQLiteError(log, _stepState, StringView("sqlite3_step: ") + sql);
        _connection->rollback(log); // (will only rollback if in an explicit transaction)
        close(log);
        return false;
    }
}

bool SQLiteDatabase::SQLiteCursor::reset(Log* log)
{
    PRIME_ASSERT(isPrepared());
    int result = sqlite3_reset(_statement);
    if (result != SQLITE_OK) {
        _statement = NULL;
        _connection->logSQLiteError(log, result, "sqlite3_reset");
        return false;
    }

    return true;
}

int SQLiteDatabase::SQLiteCursor::sqliteBind(int index, const Value& value, Log* log)
{
    PRIME_ASSERT(isPrepared());

    ++index; // sqlite is one based.

    switch (value.getType()) {
    case Value::TypeUndefined:
        PRIME_ASSERTMSG(0, "'undefined' bound to SQLiteDatabase::SQLiteCursor argument.");
        return SQLITE_ERROR;

    case Value::TypeNull:
        return sqlite3_bind_null(_statement, index);

    case Value::TypeBool:
    case Value::TypeInteger:
        return sqlite3_bind_int64(_statement, index, value.toInt64());

    case Value::TypeReal:
        return sqlite3_bind_double(_statement, index, value.toDouble());

    case Value::TypeString: {
        const std::string& string = value.getString();
        return sqlite3_bind_text(_statement, index, string.c_str(), (int)string.size(), SQLITE_TRANSIENT);
    }

    case Value::TypeData: {
        const Data& data = value.getData();
        return sqlite3_bind_blob(_statement, index, &data[0], (int)data.size(), SQLITE_TRANSIENT);
    }

    case Value::TypeDate: {
        char buffer[64];
        PRIME_EXPECT(value.getDate().toISO8601(buffer, sizeof(buffer)));
        return sqlite3_bind_text(_statement, index, buffer, -1, SQLITE_TRANSIENT);
    }

    case Value::TypeTime: {
        char buffer[64];
        PRIME_EXPECT(value.getTime().toISO8601(buffer, sizeof(buffer)));
        return sqlite3_bind_text(_statement, index, buffer, -1, SQLITE_TRANSIENT);
    }

    case Value::TypeDateTime: {
        char buffer[64];
        PRIME_EXPECT(value.getDateTime().toISO8601(buffer, sizeof(buffer), " ", ""));
        return sqlite3_bind_text(_statement, index, buffer, -1, SQLITE_TRANSIENT);
    }

    case Value::TypeVector:
    case Value::TypeDictionary: {
        // Use JSON for these.
        const std::string string = ToJSON(value);
        return sqlite3_bind_text(_statement, index, string.c_str(), (int)string.size(), SQLITE_TRANSIENT);
    }

    case Value::TypeObject: {
        Value serialised = value.toValue();
        if (value.isUndefined() || value.isObject()) {
            log->error(PRIME_LOCALISE("Can't bind object to SQLite statement because it can't be converted to a value."));
            return SQLITE_ERROR;
        }

        return sqliteBind(index, serialised, log);
    }

    default:
        PRIME_ASSERT(0);
        return SQLITE_ERROR;
    }
}

bool SQLiteDatabase::SQLiteCursor::fetch(Log* log)
{
    PRIME_ASSERT(isPrepared());

    _errorFlag = false;
    _row.clear();

    if (_stepState != SQLITE_ROW) {
        if (!step(log, "(step)")) {
            _errorFlag = true;
            return false;
        }

        if (_stepState != SQLITE_ROW) {
            // No more results
            return false;
        }
    }

    _stepState = SQLITE_OK;

    if (_columnNames.empty()) {
        for (int i = 0; i != sqlite3_column_count(_statement); ++i) {
            _columnNames.add(sqlite3_column_name(_statement, i));
        }
    }

    _row.reserve(getColumnCount());

    for (int column = 0; column != Narrow<int>(_columnNames.getColumnCount()); ++column) {
        switch (sqlite3_column_type(_statement, column)) {
        case SQLITE_INTEGER:
            _row.push_back(Value((Value::Integer)sqlite3_column_int64(_statement, column)));
            break;

        case SQLITE_FLOAT:
            _row.push_back(Value((Value::Real)sqlite3_column_double(_statement, column)));
            break;

        case SQLITE_TEXT:
            _row.push_back(Value((const char*)sqlite3_column_text(_statement, column)));
            break;

        case SQLITE_BLOB: {
            const void* data = sqlite3_column_blob(_statement, column);
            size_t size = (size_t)sqlite3_column_bytes(_statement, column);
            _row.push_back(Value(Data(data, size)));
            break;
        }

        case SQLITE_NULL:
            _row.push_back(null);
            break;

        default:
            _row.push_back(undefined);
            break;
        }
    }

    return true;
}

//
// SQLiteDatabase::SQLiteConnection implementation
//

void SQLiteDatabase::SQLiteConnection::logSQLiteError(Log* log, int result, StringView cause)
{
    log->runtimeError(MakeString("SQLite: ", cause, ": error ", result, ": ", _db ? sqlite3_errmsg(_db) : "database not open."));
}

SQLiteDatabase::SQLiteConnection::SQLiteConnection()
    : _db(NULL)
    , _verboseLogging(false)
{
}

SQLiteDatabase::SQLiteConnection::~SQLiteConnection()
{
    retain();
    close(Log::getGlobal());
}

static int DecimalCollationCallback(void*, int aLength, const void* aBytes, int bLength, const void* bBytes)
{
    const char* a = (const char*)aBytes;
    const char* aEnd = a + aLength;

    const char* b = (const char*)bBytes;
    const char* bEnd = b + bLength;

    int lessThan = -1;
    int greaterThan = 1;

    // Skip a + on either number.

    if (a != aEnd && *a == '+') {
        ++a;
    }

    if (b != bEnd && *b == '+') {
        ++b;
    }

    // Skip the -, which is also an easy first test for order.

    if (a != aEnd && *a == '-') {
        if (b == bEnd || *b != '-') {
            return lessThan;
        }

        ++a;
        ++b;

        // Switch signs.
        lessThan *= -1;
        greaterThan *= -1;
    } else if (b != bEnd && *b == '-') {
        return greaterThan;
    }

    // Skip leading zeros in the integer part.

    while (a != aEnd && *a == '0') {
        ++a;
    }

    while (b != bEnd && *b == '0') {
        ++b;
    }

    // Count the number of integer digits in both numbers.

    ptrdiff_t aIntegerSize = std::find(a, aEnd, '.') - a;
    ptrdiff_t bIntegerSize = std::find(b, bEnd, '.') - b;

    if (aIntegerSize > bIntegerSize) {
        return greaterThan;
    }
    if (aIntegerSize < bIntegerSize) {
        return lessThan;
    }

    // Compare the integer parts.

    int cmp = memcmp(a, b, (size_t)aIntegerSize);
    if (cmp != 0) {
        return cmp < 0 ? lessThan : greaterThan;
    }

    // Skip the integer parts.

    a += aIntegerSize;
    if (a != aEnd && *a == '.') {
        ++a;
    }

    b += bIntegerSize;
    if (b != bEnd && *b == '.') {
        ++b;
    }

    // Compare the fractional parts.

    cmp = LexicographicalCompare3Way(a, aEnd, b, bEnd);
    if (cmp != 0) {
        return cmp < 0 ? lessThan : greaterThan;
    }

    return 0;
}

static void DecEqFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    // TODO: throw an error if not a number, instead of value_or
    Decimal lhs = Decimal::fromString((const char*)sqlite3_value_text(argv[0])).value_or(0);
    Decimal rhs = Decimal::fromString((const char*)sqlite3_value_text(argv[1])).value_or(0);

    int eq = (lhs == rhs) ? 1 : 0;

    sqlite3_result_int(context, eq);
}

static void DecMulFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    // TODO: throw an error if not a number, instead of value_or
    Decimal lhs = Decimal::fromString((const char*)sqlite3_value_text(argv[0])).value_or(0);
    Decimal rhs = Decimal::fromString((const char*)sqlite3_value_text(argv[1])).value_or(0);

    char output[128];
    (lhs * rhs).toString(output, sizeof(output));

    sqlite3_result_text(context, output, -1, SQLITE_TRANSIENT);
}

static void DecDivFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    // TODO: throw an error if not a number, instead of value_or
    Decimal lhs = Decimal::fromString((const char*)sqlite3_value_text(argv[0])).value_or(0);
    Decimal rhs = Decimal::fromString((const char*)sqlite3_value_text(argv[1])).value_or(0);

    char output[128];
    if (rhs.isZero()) {
        StringCopy(output, sizeof(output), "0");
    } else {
        (lhs / rhs).toString(output, sizeof(output));
    }

    sqlite3_result_text(context, output, -1, SQLITE_TRANSIENT);
}

static void DecAddFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    // TODO: throw an error if not a number, instead of value_or
    Decimal lhs = Decimal::fromString((const char*)sqlite3_value_text(argv[0])).value_or(0);
    Decimal rhs = Decimal::fromString((const char*)sqlite3_value_text(argv[1])).value_or(0);

    char output[128];
    (lhs + rhs).toString(output, sizeof(output));

    sqlite3_result_text(context, output, -1, SQLITE_TRANSIENT);
}

static void DecSubFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    // TODO: throw an error if not a number, instead of value_or
    Decimal lhs = Decimal::fromString((const char*)sqlite3_value_text(argv[0])).value_or(0);
    Decimal rhs = Decimal::fromString((const char*)sqlite3_value_text(argv[1])).value_or(0);

    char output[128];
    (lhs - rhs).toString(output, sizeof(output));

    sqlite3_result_text(context, output, -1, SQLITE_TRANSIENT);
}

static void DecRoundFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv, Decimal::RoundMode roundingMode)
{
    (void)argc;
    PRIME_ASSERT(argc == 2);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    // TODO: throw an error if not a number, instead of value_or
    Decimal dec = Decimal::fromString((const char*)sqlite3_value_text(argv[0])).value_or(0);
    int places = sqlite3_value_int(argv[1]);

    char output[128];
    dec.getRounded(places, roundingMode).toString(output, sizeof(output));

    sqlite3_result_text(context, output, -1, SQLITE_TRANSIENT);
}

static void DecRoundBankersFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    DecRoundFunctionCallback(context, argc, argv, Decimal::RoundModeBankersRounding);
}

static void DecRoundUpFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    DecRoundFunctionCallback(context, argc, argv, Decimal::RoundModeHalfAwayFromZero);
}

static void DecRoundEvenFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    DecRoundFunctionCallback(context, argc, argv, Decimal::RoundModeHalfToEven);
}

static void ThousandsFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;

    const char* original = (const char*)sqlite3_value_text(argv[0]);
    const char* originalEnd = original + strlen(original);

    const char* integerEnd = std::find(original, originalEnd, '.');
    const char* integerBegin = integerEnd;

    while (integerBegin-- >= original) {
        if (!ASCIIIsDigit(*integerBegin)) {
            break;
        }
    }

    ++integerBegin;

    char output[256];
    output[0] = 0;
    StringAppend(output, sizeof(output), original, (size_t)(integerBegin - original));

    char* outputPtr = output + strlen(output);
    char* outputEnd = output + sizeof(output) - 1;

    int digitCount = (int)(integerEnd - integerBegin);
    const int digitGroup = 3;
    const char groupSeparator = ',';

    int nextSeparator = digitCount % digitGroup;
    if (!nextSeparator) {
        nextSeparator = digitGroup;
    }

    while (digitCount--) {
        if (nextSeparator == 0) {
            nextSeparator = digitGroup - 1;
            if (outputPtr < outputEnd) {
                *outputPtr = groupSeparator;
            }
            ++outputPtr;
        } else {
            --nextSeparator;
        }

        if (outputPtr < outputEnd) {
            *outputPtr = integerEnd[-1 - digitCount];
        }
        ++outputPtr;
    }

    *outputPtr = 0;

    StringAppend(output, sizeof(output), integerEnd);

    sqlite3_result_text(context, output, -1, SQLITE_TRANSIENT);
}

struct DecSumAggregateData {
    char valueStorage[sizeof(Decimal)];
    bool initialised;

    Decimal& getDecimal()
    {
        if (!initialised) {
            new (valueStorage) Decimal;
            initialised = true;
        }

        return *(Decimal*)valueStorage;
    }
};

static void DecSumAggregateStepCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    DecSumAggregateData* data = (DecSumAggregateData*)sqlite3_aggregate_context(context, sizeof(DecSumAggregateData));
    Decimal& decimal = data->getDecimal();
    for (int i = 0; i != argc; ++i) {
        // a NULL value should be treated as zero in this case
        decimal += Decimal::fromString((const char*)sqlite3_value_text(argv[i])).value_or(0);
    }
}

static void DecSumAggregateFinalCallback(sqlite3_context* context)
{
    DecSumAggregateData* data = (DecSumAggregateData*)sqlite3_aggregate_context(context, sizeof(DecSumAggregateData));

    char output[128];
    data->getDecimal().toString(output, sizeof(output));

    sqlite3_result_text(context, output, -1, SQLITE_TRANSIENT);
}

static void DateFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 1);

    Optional<DateTime> dt = DateTime::parse((const char*)sqlite3_value_text(argv[0]));
    if (!dt) {
        sqlite3_result_text(context, "(invalid DateTime)", -1, SQLITE_TRANSIENT);
        return;
    }

    Date date = dt->getDate();

    sqlite3_result_text(context, Format("%04d/%02d/%02d", date.getYear(), date.getMonth(), date.getDay()).c_str(), -1, SQLITE_TRANSIENT);
}

static void GBDateFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 1);

    Optional<DateTime> dt = DateTime::parse((const char*)sqlite3_value_text(argv[0]));
    if (!dt) {
        sqlite3_result_text(context, "(invalid DateTime)", -1, SQLITE_TRANSIENT);
        return;
    }

    Date date = dt->getDate();

    sqlite3_result_text(context, Format("%02d/%02d/%04d", date.getDay(), date.getMonth(), date.getYear()).c_str(), -1, SQLITE_TRANSIENT);
}

static void GBDateTimeFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 1);

    Optional<DateTime> dt = DateTime::parse((const char*)sqlite3_value_text(argv[0]));
    if (!dt) {
        sqlite3_result_text(context, "(invalid DateTime)", -1, SQLITE_TRANSIENT);
        return;
    }

    DateTime local = Clock::unixTimeToLocalDateTime(dt->toUnixTime());

    sqlite3_result_text(context, Format("%02d/%02d/%04d %02d:%02d:%02d", local.getDay(), local.getMonth(), local.getYear(), local.getHour(), local.getMinute(), local.getSecond()).c_str(), -1, SQLITE_TRANSIENT);
}

static void GBCurrencyFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 1);

    Optional<Decimal> decimal = Decimal::fromString((const char*)sqlite3_value_text(argv[0]));
    if (!decimal) {
        sqlite3_result_text(context, "(invalid Decimal)", -1, SQLITE_TRANSIENT);
        return;
    }

    sqlite3_result_text(context, MakeString(decimal->toString(2, Decimal::RoundModeHalfToEven)).c_str(), -1, SQLITE_TRANSIENT);
}

static void InitialsFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 1);

    StringView name = StringViewTrim((const char*)sqlite3_value_text(argv[0]));
    std::vector<std::string> words = StringSplit(name, " ");

    std::string output;

    for (size_t i = 0; i != words.size(); ++i) {
        if (!words[i].empty()) {
            output += StringToUpper(StringView(words[i]).substr(0, 1));
        }
    }

    sqlite3_result_text(context, output.c_str(), -1, SQLITE_TRANSIENT);
}

/// Returns true if the datetime argv[0] is since days (argv[1]) + months (argv[2]) of argv[3] or the current time
static void DateIsSinceFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 3 || argc == 4);

    const char* dateTimeString = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
    if (!dateTimeString) {
        sqlite3_result_int(context, 0);
        return;
    }

    Optional<DateTime> inputDateTime = DateTime::parseISO8601(dateTimeString);
    if (!inputDateTime) {
        sqlite3_result_int(context, 0);
        return;
    }

    Optional<DateTime> fromDateTime;
    if (argc >= 4) {
        if (const char* fromDateTimeString = reinterpret_cast<const char*>(sqlite3_value_text(argv[3]))) {
            fromDateTime = DateTime::parseISO8601(fromDateTimeString);
        }
    }

    DateTime newDateTime = fromDateTime.value_or(Clock::getCurrentTime());

    bool haveAnInterval = false;
    if (const char* daysString = reinterpret_cast<const char*>(sqlite3_value_text(argv[1]))) {
        haveAnInterval = true;
        UnixTime::Seconds days;
        if (StringToInt(daysString, days)) {
            if (days < 0) {
                days = 0;
            }

            // TODO: convert to unix days and back again to avoid drift due to daylight savings

            newDateTime = newDateTime.toUnixTime() - UnixTime(days * secondsPerDay);
        }
    }

    if (const char* monthsString = reinterpret_cast<const char*>(sqlite3_value_text(argv[2]))) {
        haveAnInterval = true;
        int months;
        if (StringToInt(monthsString, months)) {
            if (months < 0) {
                months = 0;
            }

            int m = newDateTime.getMonth();
            int y = newDateTime.getYear();
            m -= months;
            while (m < 1) {
                m += 12;
                y -= 1;
            }

            newDateTime.setYear(y);
            newDateTime.setMonth(m);
        }
    }

    if (!haveAnInterval) {
        sqlite3_result_int(context, 1);
        return;
    }

    sqlite3_result_int(context, (*inputDateTime > newDateTime) ? 1 : 0);
}

/// Returns true if the datetime argv[0] is since days (argv[1]) + months (argv[2]) of argv[3] or the current time
static void MiddleTruncateFunctionCallback(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc;
    PRIME_ASSERT(argc == 2 || argc == 3);

    const char* string = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
    if (!string) {
        sqlite3_result_text(context, "", -1, SQLITE_TRANSIENT);
        return;
    }

    int64_t maxSize = sqlite3_value_int64(argv[1]);
    if (maxSize < 1) {
        sqlite3_result_text(context, "", -1, SQLITE_TRANSIENT);
        return;
    }

    const char* ellipsis = NULL;
    if (argc >= 3) {
        ellipsis = reinterpret_cast<const char*>(sqlite3_value_text(argv[2]));
    }
    if (!ellipsis) {
        ellipsis = "...";
    }

    std::string buffer(string);
    MiddleTruncateStringInPlace(buffer, Narrow<size_t>(maxSize), ellipsis);

    sqlite3_result_text(context, buffer.c_str(), Narrow<int>(buffer.size()), SQLITE_TRANSIENT);
}

bool SQLiteDatabase::SQLiteConnection::open(Database* database, const char* path, const Options& options, Log* log)
{
    PRIME_ASSERT(!isOpen());
    PRIME_ASSERT(GlobalInitialisation::getSingleton()->isInitialised());

    _database = database;
    _verboseLogging = options.getVerboseLogging();

    // sqlite3 takes UTF-8 file names - Yay!

    int result = sqlite3_open_v2(path, &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (result != SQLITE_OK) {
        logSQLiteError(log, result, "sqlite3_open_v2");
        _db = NULL;
        return false;
    }

    result = sqlite3_unicode_init(_db);
    if (result != SQLITE_OK) {
        logSQLiteError(log, result, "sqlite3_unicode_init");
        sqlite3_close(_db);
        _db = NULL;
        return false;
    }

    sqlite3_busy_timeout(_db, options.getTimeoutMilliseconds());

    if (!options.getOpenForBackup()) {
        sqlite3_exec(_db, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);
    }

    sqlite3_create_collation(_db, "decimal", SQLITE_UTF8, NULL, &DecimalCollationCallback);
    sqlite3_create_collation(_db, "natural_nocase", SQLITE_UTF8, NULL, &DecimalCollationCallback); // TODO!
    sqlite3_create_function(_db, "thousands", 1, SQLITE_UTF8, NULL, &ThousandsFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "deceq", 2, SQLITE_UTF8, NULL, &DecEqFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "decmul", 2, SQLITE_UTF8, NULL, &DecMulFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "decdiv", 2, SQLITE_UTF8, NULL, &DecDivFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "decadd", 2, SQLITE_UTF8, NULL, &DecAddFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "decsub", 2, SQLITE_UTF8, NULL, &DecSubFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "decroundbankers", 2, SQLITE_UTF8, NULL, &DecRoundBankersFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "decroundup", 2, SQLITE_UTF8, NULL, &DecRoundUpFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "decroundeven", 2, SQLITE_UTF8, NULL, &DecRoundEvenFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "decsum", -1, SQLITE_UTF8, NULL, NULL, &DecSumAggregateStepCallback, &DecSumAggregateFinalCallback);
    sqlite3_create_function(_db, "date", 1, SQLITE_UTF8, NULL, &DateFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "gbdate", 1, SQLITE_UTF8, NULL, &GBDateFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "gbdatetime", 1, SQLITE_UTF8, NULL, &GBDateTimeFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "gbcurrency", 1, SQLITE_UTF8, NULL, &GBCurrencyFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "initials", 1, SQLITE_UTF8, NULL, &InitialsFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "date_is_since", 4, SQLITE_UTF8, NULL, &DateIsSinceFunctionCallback, NULL, NULL);
    sqlite3_create_function(_db, "middle_truncate", 3, SQLITE_UTF8, NULL, &MiddleTruncateFunctionCallback, NULL, NULL);

    return true;
}

bool SQLiteDatabase::SQLiteConnection::inTransaction()
{
    PRIME_ASSERT(isOpen());

    return sqlite3_get_autocommit(_db) == 0;
}

bool SQLiteDatabase::SQLiteConnection::close(Log* log)
{
    bool success = true;

    if (_db) {
        if (inTransaction()) {
            if (!rollback(log)) {
                success = false;
            }
        }

        sqlite3_close(_db);
        _db = NULL;
    }

    return success;
}

RefPtr<Cursor> SQLiteDatabase::SQLiteConnection::createCursor(Log*, const CreateCursorOptions& options)
{
    PRIME_ASSERT(isOpen());

    return PassRef(new SQLiteCursor(this, options.getAutomaticTransactions()));
}

bool SQLiteDatabase::SQLiteConnection::needTransaction(Log* log)
{
    if (inTransaction()) {
        return true;
    }

    SQLiteCursor cursor(this, false);
    return cursor.execute("BEGIN IMMEDIATE TRANSACTION", log) && cursor.close(log);
}

bool SQLiteDatabase::SQLiteConnection::commit(Log* log)
{
    return endTransaction("COMMIT", log);
}

bool SQLiteDatabase::SQLiteConnection::endTransaction(const char* statement, Log* log)
{
    if (!inTransaction()) {
        return true;
    }

    SQLiteCursor cursor(this, false);
    return cursor.execute(statement, log) && cursor.close(log);
}

bool SQLiteDatabase::SQLiteConnection::rollback(Log* log)
{
    return endTransaction("ROLLBACK", log);
}

void SQLiteDatabase::SQLiteConnection::appendQuoted(std::string& output, StringView string, bool isWildcard) const
{
    SQLSyntax::getSQLiteSyntax()->appendQuoted(output, string, isWildcard);
}

void SQLiteDatabase::SQLiteConnection::appendEscaped(std::string& output, StringView string, bool isWildcard,
    const char** wildcardModifier) const
{
    SQLSyntax::getSQLiteSyntax()->appendEscaped(output, string, isWildcard, wildcardModifier);
}

//
// SQLiteDatabase::GlobalInitialisation
//

SQLiteDatabase::GlobalInitialisation* SQLiteDatabase::GlobalInitialisation::getSingleton()
{
    static GlobalInitialisation singleton;
    return &singleton;
}

SQLiteDatabase::GlobalInitialisation::GlobalInitialisation()
    : _initialised(0)
{
    init();
}

SQLiteDatabase::GlobalInitialisation::~GlobalInitialisation()
{
    shutdown();
}

void SQLiteDatabase::GlobalInitialisation::init()
{
    if (_initialised++) {
        return;
    }

    if (sqlite3_config(SQLITE_CONFIG_MULTITHREAD) != SQLITE_OK) {
        RuntimeError("Couldn't configure SQLite for multithreading.");
    }

    int result = sqlite3_unicode_load();
    if (result != SQLITE_OK) {
        RuntimeError("sqlite3_unicode_load failed: %d.", result);
    }
}

void SQLiteDatabase::GlobalInitialisation::shutdown()
{
    PRIME_ASSERT(_initialised > 0);
    if (!--_initialised) {
        return;
    }

    sqlite3_unicode_free();
}

//
// SQLiteDatabase
//

PRIME_DEFINE_UID_CAST(SQLiteDatabase)

bool SQLiteDatabase::isCompleteCommand(const char* command)
{
    return sqlite3_complete(command);
}

SQLiteDatabase::SQLiteDatabase()
{
    globalInit();
}

SQLiteDatabase::~SQLiteDatabase()
{
}

bool SQLiteDatabase::init(std::string dbPath, const Options& options, Log* log)
{
    (void)log;

    _dbPath.swap(dbPath);
    _options = options;

    return true;
}

bool SQLiteDatabase::compact(Log* log)
{
    SQLiteConnection connection;
    if (!connection.open(this, _dbPath.c_str(), _options, log)) {
        return false;
    }

    SQLiteCursor cursor(&connection, true);
    if (!cursor.execute("PRAGMA wal_checkpoint", log)) {
        return false;
    }

    return true;
}

bool SQLiteDatabase::integrityCheck(Log* log)
{
    SQLiteConnection connection;
    if (!connection.open(this, _dbPath.c_str(), _options, log)) {
        return false;
    }

    char* errmsg = NULL;
    sqlite3_exec(connection.getSQLite3(), "PRAGMA integrity_check(999999)", &IntegrityCheckCallback, reinterpret_cast<void*>(log), &errmsg);

    return true;
}

bool SQLiteDatabase::backup(DatabaseConnection* dest, DatabaseConnection* source, Log* log, int pagesPerBatch, int pauseMillisecondsBetweenBatches)
{
    sqlite3_backup* backup = sqlite3_backup_init((sqlite3*)dest->getHandle(), "main",
        (sqlite3*)source->getHandle(), "main");
    if (!backup) {
        log->error(PRIME_LOCALISE("Unable to create sqlite3 backup object."));
        return false;
    }

    bool success = true;

    for (;;) {
        int rc = sqlite3_backup_step(backup, pagesPerBatch);

        if (rc == SQLITE_DONE) {
            break;
        }

        int pageCount = sqlite3_backup_pagecount(backup);
        int remaining = sqlite3_backup_remaining(backup);
        log->trace("Backing up database (%d/%d pages)", pageCount - remaining, pageCount);

        if (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
            if (pauseMillisecondsBetweenBatches > 0) {
                Clock::sleepMilliseconds((unsigned int)pauseMillisecondsBetweenBatches);
            }
        } else {
            log->error(PRIME_LOCALISE("SQLite3 backup error (%d)."), rc);
            success = false;
            break;
        }
    }

    if (sqlite3_backup_finish(backup) != SQLITE_OK) {
        log->error(PRIME_LOCALISE("Unable to complete sqlite3 backup."));
        success = false;
    }

    return success;
}

RefPtr<DatabaseConnection> SQLiteDatabase::connect(Log* log)
{
    RefPtr<SQLiteConnection> connection = PassRef(new SQLiteConnection);
    if (!connection->open(this, _dbPath.c_str(), _options, log)) {
        return NULL;
    }

    return connection;
}

const SQLSyntax* SQLiteDatabase::getSQLSyntax() const
{
    return SQLSyntax::getSQLiteSyntax();
}
}

#endif // PRIME_HAVE_SQLITEDATABASE
