// Copyright 2000-2021 Mark H. P. Lord

#include "StandardApp.h"
#include "DefaultLog.h"
#include "File.h"
#include "FileLoader.h"
#include "FileLocations.h"
#include "FileProperties.h"
#include "NumberUtils.h"
#include "Path.h"
#include "SystemFileSystem.h"
#include "ZipFileSystem.h"

namespace Prime {

StandardApp::StandardApp(int* argc, char*** argv, Log* log)
    : _commandLine(argc, argv)
{
    construct(log);
}

StandardApp::StandardApp(int argc, char** argv, Log* log)
    : _commandLine(argc, argv)
{
    construct(log);
}

static char* fakeArgv[] = { (char*)"" };

StandardApp::StandardApp(Log* log)
    : _commandLine(1, fakeArgv)
{
    construct(log);
}

void StandardApp::construct(Log* log)
{
    // Do this early before we've chdir()d
    _executablePath = GetExecutableFilePath(_commandLine.getArgv()[0], log);

    _clearFileLog = false;

    if (log) {
        _appLog = log;
    } else {
        _defaultLog.reset(new DefaultLog);
        _appLog = _defaultLog.get();
    }

    if (TextLog* textLog = UIDCast<TextLog>(_appLog)) {
        textLog->setApplicationName(_commandLine.getArgv()[0]);
    }

    _started = false;

    _multiLog.addLog(_appLog);

    Log::setGlobal(&_multiLog);

    _commandLineOptions.logPath = "";
    _commandLineOptions.taskSystemName = "";
    _commandLineOptions.useLogThreader = -1;
    _commandLineOptions.settingsPath = "";
    _commandLineOptions.dataPath = "";
    _commandLineOptions.toolsPath = "";
    _commandLineOptions.maxConcurrentThreads = -1;
}

StandardApp::~StandardApp()
{
    close(false);
}

void StandardApp::init(Options options)
{
    _options = PRIME_MOVE(options);

    if (options.getUseStdout()) {
        if (ConsoleLog* consoleLog = UIDCast<ConsoleLog>(_appLog)) {
            consoleLog->setUseStdoutForAllLevels();
        }
    }
}

CommandLineParser& StandardApp::getCommandLineParser()
{
    if (TextLog* textLog = UIDCast<TextLog>(_appLog)) {
        textLog->setLevel(Log::LevelOutput);
    }
    _commandLineParser.init(_commandLine.getArgv());
    _commandLineParser.setResponseFileLoader('@', &_responseFileLoader);
    _commandLineParser.setImplicitLongOptionsEnabled(true);
    return _commandLineParser;
}

void StandardApp::setGlobalLog(Log* log)
{
    _multiLog.replace(_appLog, log);
    _appLog = log;
}

bool StandardApp::processCommandLineOption(bool exitIfUnknown)
{
    CommandLineParser& cl = _commandLineParser;

    if (cl.readOption("help|h|?")) {
        help();
        exit(EXIT_SUCCESS);
    }

    if (cl.readOption("verbose|v")) {
        if (TextLog* textLog = UIDCast<TextLog>(_appLog)) {
            textLog->increaseVerbosity();
        }
        return true;
    }

    if (cl.readOption("developer")) {
        SetDeveloperMode(true);
        return true;
    }

    if (cl.readValue("log")) {
        _commandLineOptions.logPath = cl.fetchString();
        return true;
    }

    if (cl.readColourFlag()) {
        if (ConsoleLog* consoleLog = UIDCast<ConsoleLog>(_appLog)) {
            consoleLog->setColourEnabled(cl.getFlag());
            return true;
        }
    }

    if (cl.readValue("task-sytem|job-system")) {
        _commandLineOptions.taskSystemName = cl.fetchString();
        return true;
    }

    if (cl.readFlag("log-threader")) {
        _commandLineOptions.useLogThreader = cl.getFlag() ? 1 : 0;
        return true;
    }

    if (cl.readValue("tools")) {
        _commandLineOptions.toolsPath = cl.fetchString();
        return true;
    }

    if (cl.readValue("data|resources|resource")) {
        _commandLineOptions.dataPath = cl.fetchString();
        return true;
    }

    if (cl.readValue("save")) {
        _commandLineOptions.settingsPath = cl.fetchString();
        return true;
    }

    if (cl.readValue("settings|config")) {
        _commandLineOptions.settingsPath = cl.fetchString();
        return true;
    }

    if (cl.isOption()) {
        if (exitIfUnknown) {
            exitDueToUnknownCommandLineOption();
        }

        return false;
    }

    if (cl.getFilename()) {
        if (!cl.hasOptionTerminatorBeenRead() && cl.getFilename()[0] == '+') {
            const char* name = cl.getFilename() + 1;
            const char* value = cl.fetchString();
            _commandLineSettings.set(ASCIIToLower(name), value);
            return true;
        }

        if (_options.getSettingsMode() == SettingsArgument && _commandLineOptions.settingsPath.empty()) {
            _commandLineOptions.settingsPath = cl.getFilename();
            return true;
        }

        if (_options.getDataPathMode() == DataPathArgument && _commandLineOptions.dataPath.empty()) {
            _commandLineOptions.dataPath = cl.getFilename();
            return true;
        }
    }

    if (exitIfUnknown) {
        exitDueToUnknownCommandLineOption();
    }

    return false;
}

void StandardApp::exitDueToUnknownCommandLineOption()
{
    _commandLineParser.exitDueToUnknownOptionOrUnexpectedArgument();
}

void StandardApp::startFileLog()
{
    if (!_commandLineOptions.logPath.empty()) {
        if (!NormalisePath(_logPath, _commandLineOptions.logPath.c_str(), Log::getGlobal())) {
            getLog()->warning(PRIME_LOCALISE("Failed to normalise log path."));
            _logPath = _commandLineOptions.logPath;
        }

        if (!_fileLog.init(_logPath.c_str(), Log::getGlobal(), FileLog::Options().setUnlimitedFileSize())) {
            getLog()->warning(PRIME_LOCALISE("Unable to create log file: %s"), _logPath.c_str());

        } else {

            _fileLog.setLevel(Log::LevelTrace);
            _multiLog.addLog(&_fileLog);

            if (_clearFileLog) {
                clearLogs();
            }
        }
    }

    Trace("%s %s (" PRIME_PLATFORM_DESCRIPTION ")", _options.getName().c_str(), _options.getVersion().c_str());

    {
        Value::Vector clargs;
        clargs.reserve(_commandLine.getArgc() + 1);
        for (const char* const* p = _commandLine.getArgv(); *p; ++p) {
            clargs.push_back(*p);
        }

        Trace(MakeString("Command line: ", clargs));
    }

    Trace(MakeString("Command line settings: ", _commandLineSettings));
}

void StandardApp::startDataFileSystem()
{
    if (_commandLineOptions.dataPath.empty()) { // TODO: check this before starting any services?
        if (_options.getDataPathMode() == DataPathOptional) {
            return;
        }

        help();
        getLog()->exitError(PRIME_LOCALISE("Missing data path."));
    }

    if (!NormalisePath(_dataPath, _commandLineOptions.dataPath.c_str(), Log::getGlobal())) {
        getLog()->warning(PRIME_LOCALISE("Failed to normalise data path."));
        _dataPath = _commandLineOptions.dataPath;
    }

#ifdef PRIME_HAVE_ZIPFILESYSTEM
    _dataFileSystem = ZipFileSystem::createFileSystemForZipOrDirectory(_dataPath.c_str(), Log::getGlobal());
    if (!_dataFileSystem) {
        getLog()->exitError(PRIME_LOCALISE("Can't open data."));
    }
#else
    Log::getGlobal()->trace("Mounting directory: %s", _dataPath.c_str());
    RefPtr<SystemFileSystem> staticFileSystem = PassRef(new SystemFileSystem);
    staticFileSystem->setPath(_dataPath.c_str());

    _dataFileSystem = staticFileSystem;
#endif
}

void StandardApp::startSettingsPath()
{
    std::string settingsPath;

    if (!_commandLineOptions.settingsPath.empty()) {
        settingsPath = _commandLineOptions.settingsPath;
        FileProperties fileProperties;
        if (!fileProperties.read(settingsPath.c_str(), Log::getNullLog()) || fileProperties.isDirectory()) {
            if (!_options.getSettingsFilename().empty()) {
                settingsPath = Path::join(settingsPath, _options.getSettingsFilename());
            }
        }

    } else {
        if ((_options.getSettingsMode() == SettingsDefaultToSystemLocation) && !_options.getSettingsFilename().empty()) {
            std::string location = GetSavePath(_options.getAppID().c_str(), Log::getGlobal());
            if (!location.empty()) {
                settingsPath = Path::join(location, _options.getSettingsFilename());
            }
        }
    }

    if (!settingsPath.empty()) {
        MakePathToFile(settingsPath.c_str(), Log::getNullLog());

        if (!NormalisePath(_settingsFilePath, settingsPath.c_str(), Log::getGlobal())) {
            getLog()->warning(PRIME_LOCALISE("Failed to normalise settings file path."));
            _settingsFilePath = settingsPath;
        }

        _savePath = Path::stripLastComponent(_settingsFilePath);

    } else if (_options.getSettingsMode() != SettingsOptional) {
        getLog()->exitError(PRIME_LOCALISE("Settings path is required."));
    }
}

void StandardApp::startSaveFileSystem()
{
    if (!_savePath.empty()) {
        _saveFileSystem = PassRef(new SystemFileSystem(_savePath.c_str()));
    }
}

void StandardApp::startSettings()
{
    JSONWriter::Options settingsWriterOptions;
    // Not actually JSON if we put a comment in there
    if (!_settingsStore.init(PassRef(new SystemFileSystem), _settingsFilePath.c_str(), _options.getSettingsFileMustExist(),
            _dataFileSystem, _dataFileSystem ? "defaults.json" : "", _defaults,
            Log::getGlobal(), _commandLineSettings, settingsWriterOptions)) {
        getLog()->exitError(PRIME_LOCALISE("Unable to initialise settings store."));
    }

    _settings = _settingsStore.getSettings();
}

void StandardApp::startFileSystem()
{
    if (_saveFileSystem) {
        _fileSystem.addFileSystem(_saveFileSystem);
    }

    if (_dataFileSystem) {
        _fileSystem.addFileSystem(_dataFileSystem);
    }

    if (_saveFileSystem) {
        _fileSystem.setWritableFileSystem(_saveFileSystem);
    }
}

void StandardApp::startFixUpFileLog()
{
    // Fix the FileLog's settings, now settings are available
    FileLog::Options defaults;
    _fileLog.getOptions().setMaxFileSize(getSettings()->get("maxLogFileSize").toInt64(defaults.getMaxFileSize()));
    _fileLog.getOptions().setMaxFiles(getSettings()->get("maxLogFiles").toInt(defaults.getMaxFiles()));
}

void StandardApp::startTaskSystem()
{
    int maxConcurrentThreads = _commandLineOptions.maxConcurrentThreads <= 0
        ? getSettings()->get("threads").toInt(computeDefaultMaxConcurrentThreads())
        : _commandLineOptions.maxConcurrentThreads;

    std::string taskSystemName = _commandLineOptions.taskSystemName.empty()
        ? ToString(getSettings()->get("taskSystem").otherwise(getSettings()->get("jobSystem")))
        : _commandLineOptions.taskSystemName;

    _taskSystemSelector.reset(new TaskSystemSelector);
    _taskSystemSelector->select(taskSystemName.c_str());
    if (!_taskSystemSelector->init(maxConcurrentThreads, 0, 0, Log::getGlobal())) {
        getLog()->exitError(PRIME_LOCALISE("Unable to initialise task system."));
    }
}

int StandardApp::computeDefaultMaxConcurrentThreads() const
{
    int threadsPerCPU = getSettings()->get("threadsPerCPU").toInt(1);
    int minThreads = getSettings()->get("minThreads").toInt(1);
    int maxThreads = getSettings()->get("maxThreads").toInt(64);

    return Clamp(threadsPerCPU * Max(Thread::getCPUCount(Log::getGlobal()), 1), minThreads, maxThreads);
}

void StandardApp::startToolsPath()
{
    if (!_commandLineOptions.toolsPath.empty()) {
        if (!NormalisePath(_toolsPath, _commandLineOptions.toolsPath.c_str(), Log::getGlobal())) {
            getLog()->warning(PRIME_LOCALISE("Failed to normalise tools path."));
            _toolsPath = _commandLineOptions.toolsPath;
        }

    } else {
        std::string toolsPath = GetToolsPath(_commandLine.getArgv()[0], Log::getGlobal());
        Trace("Tools path: %s", toolsPath.c_str());
        if (!NormalisePath(_toolsPath, toolsPath.c_str(), Log::getGlobal())) {
            getLog()->warning(PRIME_LOCALISE("Failed to normalise tools path: %s -> %s"), toolsPath.c_str(), _toolsPath.c_str());
            _toolsPath = toolsPath;
        }
    }
}

void StandardApp::startSignalHandling()
{
    _terminationHandler.setPipeCallback(&TerminationHandler::ignoringCallback);
}

void StandardApp::startLogThreader()
{
    if (_commandLineOptions.useLogThreader == 1 || (_commandLineOptions.useLogThreader < 0 && !GetDeveloperMode())) {
        if (!_logThreader.init(&_multiLog)) {
            getLog()->exitError(PRIME_LOCALISE("Unable to initialise log threader."));
        }

        Log::setGlobal(&_logThreader);
    }
}

void StandardApp::start()
{
    PRIME_ASSERT(!_started);

    startFileLog();

    startDataFileSystem();

    startSettingsPath();

    startSaveFileSystem();

    startSettings();

    startFileSystem();

    startFixUpFileLog();

    startTaskSystem();

    startToolsPath();

    startSignalHandling();

    startLogThreader();

    _started = true;
}

void StandardApp::close(bool saveSettings)
{
    if (!_started) {
        return;
    }

    _taskSystemSelector.reset();

    if (saveSettings) {
        _settingsStore.close();
    }

    _started = false;
}

void StandardApp::help()
{
    const ArrayView<const char* const>& helpText = _options.getHelpText();

    for (unsigned i = 0; i != helpText.size(); ++i) {
        getLog()->output(helpText[i], _commandLine.getArgv()[0]);
    }
}

void StandardApp::setDefault(const char* path, Value value)
{
    Value::setDictionaryPath(_defaults, path, PRIME_MOVE(value));
}

void StandardApp::clearLogs()
{
    if (_fileLog.isInitialised()) {
        _fileLog.clearLogs(_appLog);
        _clearFileLog = false;
    } else {
        _clearFileLog = true;
    }
}
}
