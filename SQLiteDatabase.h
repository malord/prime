// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SQLITEDATABASE_H
#define PRIME_SQLITEDATABASE_H

#include "Database.h"

#ifndef PRIME_NO_SQLITE

#define PRIME_HAVE_SQLITEDATABASE

namespace Prime {

/// SQLite implementation of Database.
class PRIME_PUBLIC SQLiteDatabase : public Database {
    PRIME_DECLARE_UID_CAST(Database, 0x135556f4, 0x28974037, 0x93a1ba75, 0xfb6aef4b)

public:
    /// Doesn't need to be called under C++11.
    static void globalInit() { GlobalInitialisation::getSingleton(); }

    static void globalShutdown() { GlobalInitialisation::getSingleton()->shutdown(); }

    static bool isCompleteCommand(const char* command);

    SQLiteDatabase();

    virtual ~SQLiteDatabase();

    class PRIME_PUBLIC Options : public Database::Options {
    public:
        Options()
            : _verboseLogging(false)
            , _timeoutMilliseconds(30000)
            , _openForBackup(false)
        {
        }

        Options& setVerboseLogging(bool value)
        {
            _verboseLogging = value;
            return *this;
        }
        bool getVerboseLogging() const { return _verboseLogging; }

        Options& setTimeoutMilliseconds(int value)
        {
            _timeoutMilliseconds = value;
            return *this;
        }
        int getTimeoutMilliseconds() const { return _timeoutMilliseconds; }

        Options& setOpenForBackup(bool value = true)
        {
            _openForBackup = value;
            return *this;
        }
        bool getOpenForBackup() const { return _openForBackup; }

    private:
        bool _verboseLogging;
        int _timeoutMilliseconds;
        bool _openForBackup;
    };

    /// Doesn't modify the database or check it exists.
    bool init(std::string dbPath, const Options& options, Log* log);

    /// Issues a wal_checkpoint pragma to flush the WAL file back to the database. The WAL file can grow
    /// unexpectedly large, so this needs doing at startup since SQLite's automatic checkpointing appears broken.
    bool compact(Log* log);

    bool backup(DatabaseConnection* dest, DatabaseConnection* source, Log* log, int pagesPerBatch = -1,
        int pauseMillisecondsBetweenBatches = 0);

    bool integrityCheck(Log* log);

    const Options& getOptions() const { return _options; }

    // Database implementation.
    virtual RefPtr<DatabaseConnection> connect(Log* log) PRIME_OVERRIDE;
    virtual const SQLSyntax* getSQLSyntax() const PRIME_OVERRIDE;

private:
    class SQLiteConnection;
    class SQLiteCursor;

    std::string _dbPath;
    Options _options;

    /// SQLiteDatabase embeds one of these to ensure global initialisation is performed, but if your application
    /// is going to be creating many SQLiteDatabases then you should construct one yourself to ensure thread
    /// safety.
    class PRIME_PUBLIC GlobalInitialisation {
    public:
        static GlobalInitialisation* getSingleton();

        bool isInitialised() { return _initialised > 0; }

        void init();

        void shutdown();

    private:
        GlobalInitialisation();

        ~GlobalInitialisation();

        volatile int _initialised;
    };
};
}

#endif // PRIME_NO_SQLITE

#endif
