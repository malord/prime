// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MYSQLDATABASE_H
#define PRIME_MYSQLDATABASE_H

#include "Database.h"

#ifndef PRIME_NO_MYSQL

#define PRIME_HAVE_MYSQLDATABASE

namespace Prime {

/// MariaDB implementation of Database (requires mariadb-connector-c, which is LGPL, whereas MySQL is GPL).
class PRIME_PUBLIC MySQLDatabase : public Database {
    PRIME_DECLARE_UID_CAST(Database, 0x5302f85b, 0xad884248, 0x80295892, 0x8dd69892)

public:
    /// Doesn't need to be called under C++11.
    static void globalInit() { GlobalInitialisation::getSingleton(); }

    static void globalShutdown() { GlobalInitialisation::getSingleton()->shutdown(); }

    MySQLDatabase();

    virtual ~MySQLDatabase();

    class PRIME_PUBLIC Options : public Database::Options {
    public:
        Options()
            : _verboseLogging(false)
            , _port(0)
            , _clientFlags(0)
        {
        }

        Options& setVerboseLogging(bool value)
        {
            _verboseLogging = value;
            return *this;
        }
        bool getVerboseLogging() const { return _verboseLogging; }

        Options& setHost(std::string value)
        {
            _host.swap(value);
            return *this;
        }
        const std::string& getHost() const { return _host; }

        Options& setUser(std::string value)
        {
            _user.swap(value);
            return *this;
        }
        const std::string& getUser() const { return _user; }

        Options& setPassword(std::string value)
        {
            _password.swap(value);
            return *this;
        }
        const std::string& getPassword() const { return _password; }

        Options& setDatabaseName(std::string value)
        {
            _databaseName.swap(value);
            return *this;
        }
        const std::string& getDatabaseName() const { return _databaseName; }

        Options& setUnixSocket(std::string value)
        {
            _unixSocket.swap(value);
            return *this;
        }
        const std::string& getUnixSocket() const { return _unixSocket; }

        Options& setPort(unsigned int value)
        {
            _port = value;
            return *this;
        }
        unsigned int getPort() const { return _port; }

        Options& setClientFlags(unsigned int value)
        {
            _clientFlags = value;
            return *this;
        }
        unsigned long getClientFlags() const { return _clientFlags; }

    private:
        bool _verboseLogging;

        std::string _host;
        std::string _user;
        std::string _password;
        std::string _databaseName;
        std::string _unixSocket;
        unsigned int _port;
        unsigned long _clientFlags;
    };

    /// Doesn't modify the database or check it exists.
    bool init(const Options& options, Log* log);

    const Options& getOptions() const { return _options; }

    // Database implementation.
    virtual RefPtr<DatabaseConnection> connect(Log* log) PRIME_OVERRIDE;
    virtual const SQLSyntax* getSQLSyntax() const PRIME_OVERRIDE;

private:
    const char* getHost() const { return _options.getHost().c_str(); }
    const char* getUser() const { return _options.getUser().c_str(); }
    const char* getPassword() const { return _options.getPassword().c_str(); }
    const char* getDatabaseName() const { return _options.getDatabaseName().c_str(); }
    const char* getUnixSocketOrNull() const { return _options.getUnixSocket().empty() ? NULL : _options.getUnixSocket().c_str(); }
    unsigned int getPort() const { return _options.getPort(); }
    unsigned long getClientFlags() const { return _options.getClientFlags(); }

    class MySQLConnection;
    class MySQLCursor;

    Options _options;

    /// MySQLDatabase embeds one of these to ensure global initialisation is performed, but if your application
    /// is going to be creating many MySQLDatabases then you should construct one yourself to ensure thread
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

#endif // PRIME_NO_MYSQL

#endif
