// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STANDARDAPP_H
#define PRIME_STANDARDAPP_H

#include "CommandLineRecoder.h"
#include "FileLog.h"
#include "FileSettingsStore.h"
#include "JSONReader.h"
#include "JSONWriter.h"
#include "LogThreader.h"
#include "MultiFileSystem.h"
#include "MultiLog.h"
#include "ResponseFileLoader.h"
#include "TaskSystemSelector.h"
#include "TerminationHandler.h"

namespace Prime {

/// Initialise all the common application services supplied by Prime (logging, settings, virtual file system,
/// task queues).
class PRIME_PUBLIC StandardApp {
public:
    class LogBridge {
    public:
        LogBridge(StandardApp& app)
            : _app(app)
        {
        }

        Log* operator->() { return _app.getLog(); }

        operator Log*() const { return _app.getLog(); }

        Log& operator*() const { return *_app.getLog(); }

    private:
        StandardApp& _app;
    };

    enum SettingsMode {
        /// Settings can be specified with a --settings command line argument, otherwise none are loaded.
        /// This is the default.
        SettingsOptional,

        /// Settings default to the system location, but can be overridden by --settings on the command line.
        /// Requires a non-empty settingsFilename to be passed to setAppStrings().
        SettingsDefaultToSystemLocation,

        /// The settings file path must be the first non-option argument on the command line.
        SettingsArgument
    };

    enum DataPathMode {
        /// Data path can be specified with a --data command line argument, otherwise it won't be available.
        DataPathOptional,

        // TODO: DataDefaultToSystemLocation? We'd need a path to setAppStrings().

        /// The data path must be the first non-option argument on the command line.
        DataPathArgument
    };

    class PRIME_PUBLIC Options {
    public:
        Options(const char* name, const char* version, const char* appID, const char* settingsFilename = "")
            : _name(name)
            , _version(version)
            , _appID(appID)
            , _settingsFilename(settingsFilename)
            , _settingsMode(SettingsOptional)
            , _dataPathMode(DataPathOptional)
            , _useStdout(false)
        {
            if (!_settingsFilename.empty()) {
                _settingsMode = SettingsDefaultToSystemLocation;
            }
        }

        Options& setName(const char* value)
        {
            _name = value;
            return *this;
        }
        const std::string& getName() const PRIME_NOEXCEPT { return _name; }

        Options& setVersion(const char* value)
        {
            _version = value;
            return *this;
        }
        const std::string& getVersion() const PRIME_NOEXCEPT { return _version; }

        Options& setAppID(const char* value)
        {
            _appID = value;
            return *this;
        }
        const std::string& getAppID() const PRIME_NOEXCEPT { return _appID; }

        Options& setSettingsFilename(const char* value)
        {
            _settingsFilename = value;
            return *this;
        }
        const std::string& getSettingsFilename() const PRIME_NOEXCEPT { return _settingsFilename; }

        Options& setSettingsMode(SettingsMode settingsMode, bool fileMustExist = false)
        {
            _settingsMode = settingsMode;
            _settingsFileMustExist = fileMustExist;
            return *this;
        }

        SettingsMode getSettingsMode() const { return _settingsMode; }
        bool getSettingsFileMustExist() const { return _settingsFileMustExist; }

        Options& setDataPathMode(DataPathMode value)
        {
            _dataPathMode = value;
            return *this;
        }
        DataPathMode getDataPathMode() const { return _dataPathMode; }

        Options& setHelpText(ArrayView<const char* const> lines)
        {
            _helpText = lines;
            return *this;
        }
        const ArrayView<const char* const>& getHelpText() const { return _helpText; }

        Options& setUseStdout(bool value = true)
        {
            _useStdout = value;
            return *this;
        }
        bool getUseStdout() const { return _useStdout; }

    private:
        std::string _name;
        std::string _version;
        std::string _appID;
        std::string _settingsFilename;

        SettingsMode _settingsMode;
        DataPathMode _dataPathMode;
        bool _settingsFileMustExist;

        ArrayView<const char* const> _helpText;

        bool _useStdout;

        friend class StandardApp;
        Options() { }
    };

    StandardApp(int* argc, char*** argv, Log* log = NULL);

    StandardApp(int argc, char** argv, Log* log = NULL);

    /// If the application is running in a GUI or under another framework where command line options are read
    /// for you, use this constructor.
    explicit StandardApp(Log* log = NULL);

    ~StandardApp();

    //
    // init() - call this first.
    //

    /// Call this first.
    void init(Options options);

    //
    // Configuration. Call these methods before calling start().
    //

    void setSettingsPath(std::string value) { _commandLineOptions.settingsPath.swap(value); }

    void setDataPath(std::string value) { _commandLineOptions.dataPath.swap(value); }

    void setToolsPath(std::string value) { _commandLineOptions.toolsPath.swap(value); }

    /// Doesn't terminate the process.
    void help();

    void setQuitCallbacks(const TerminationHandler::Callback& callback)
    {
        _terminationHandler.setQuitCallbacks(callback);
    }

    TerminationHandler& getTerminationHandler() PRIME_NOEXCEPT { return _terminationHandler; }

    CommandLineRecoder& getCommandLine() PRIME_NOEXCEPT { return _commandLine; }

    /// Call this to get a CommandLineParser to read your application's options. Unknown options should be
    /// passed to processCommandLineOption(), to make sure that built-in options are handled.
    CommandLineParser& getCommandLineParser();

    /// Call this after checking for the application's own options.
    bool processCommandLineOption(bool exitIfUnknown = true);

    /// Call this to exit if you get an unknown option or unexpected filename from the CommandLineParser.
    void exitDueToUnknownCommandLineOption();

    /// If an application doesn't have a defaults file, settings specified through this method will be
    /// used instead. If the application does have a defaults file, these settings are ignored.
    void setDefault(const char* path, Value value);

    /// Replace the topmost log with another. This is necessary to make sure that the FileLog still works.
    void setGlobalLog(Log* log);

    //
    // start()
    //

    /// Start up the application once it's been initialised and the command line has been parsed.
    void start();

    //
    // Accessors. Call these after calling start().
    //

    Log* getLog() const PRIME_NOEXCEPT { return Log::getGlobal(); }

    TaskSystem* getTaskSystem() const PRIME_NOEXCEPT { return TaskSystem::getGlobal(); }

    Settings* getSettings() const PRIME_NOEXCEPT { return _settings; }

    FileSystem* getDataFileSystem() const PRIME_NOEXCEPT { return _dataFileSystem; }

    FileSystem* getSaveFileSystem() const PRIME_NOEXCEPT { return _saveFileSystem; }

    FileSystem* getFileSystem() const PRIME_NOEXCEPT { return &_fileSystem; }

    const std::string& getDataPath() const PRIME_NOEXCEPT { return _dataPath; }

    const std::string& getSavePath() const PRIME_NOEXCEPT { return _savePath; }

    const std::string& getSettingsFilePath() const PRIME_NOEXCEPT { return _settingsFilePath; }

    const std::string& getLogPath() const PRIME_NOEXCEPT { return _logPath; }

    const std::string& getToolsPath() const PRIME_NOEXCEPT { return _toolsPath; }

    const std::string& getExecutablePath() const PRIME_NOEXCEPT { return _executablePath; }

    //
    // Utilities which can be called as required
    //

    void clearLogs();

    //
    // Shutdown
    //

    /// Shut down the application, optionally saving settings.
    void close(bool saveSettings = true);

protected:
    void startFileLog();
    void startDataFileSystem();
    void startSettingsPath();
    void startSaveFileSystem();
    void startSettings();
    void startFileSystem();
    void startFixUpFileLog();
    void startTaskSystem();
    void startToolsPath();
    void startSignalHandling();
    void startLogThreader();

private:
    void construct(Log* log);

    int computeDefaultMaxConcurrentThreads() const;

    // This must be our first member.
    CommandLineRecoder _commandLine;

    Options _options;

    Log* _appLog;
    ScopedPtr<Log> _defaultLog;
    FileLog _fileLog;
    MultiLog _multiLog;
    LogThreader _logThreader;

    ResponseFileLoader _responseFileLoader;
    CommandLineParser _commandLineParser;

    bool _clearFileLog;

    // Options read from the command line.
    struct CommandLineOptions {
        CommandLineOptions() PRIME_NOEXCEPT { }

        std::string logPath;
        std::string taskSystemName;
        int useLogThreader; // -1 (default) = if not developer mode, 0 = no, 1 = yes
        std::string settingsPath;
        std::string dataPath;
        std::string toolsPath;
        int maxConcurrentThreads;
    } _commandLineOptions;

    Value::Dictionary _commandLineSettings;
    Value::Dictionary _defaults;

    // Derived from the command line.
    std::string _dataPath;
    std::string _settingsFilePath;
    std::string _savePath;
    std::string _logPath;
    std::string _toolsPath;
    std::string _executablePath;

    // The objects we manage.
    ScopedPtr<TaskSystemSelector> _taskSystemSelector;
    RefPtr<FileSystem> _dataFileSystem;
    RefPtr<FileSystem> _saveFileSystem;
    mutable MultiFileSystem _fileSystem;
    FileSettingsStore<JSONReader, JSONWriter> _settingsStore;
    RefPtr<Settings> _settings;
    TerminationHandler _terminationHandler;

    bool _started;
};
}

#endif
