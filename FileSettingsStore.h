// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_FILESETTINGSSTORE_H
#define PRIME_FILESETTINGSSTORE_H

#include "DictionarySettingsStore.h"
#include "DowngradeLog.h"
#include "FileSystem.h"
#include "PrefixLog.h"
#include "StringUtils.h"

namespace Prime {

/// A wrapper around DictionarySettingsStore which reads/writes the settings from/to a file using any compatible
/// reader and writer classes (e.g., you could use a PropertyListReader and a JSONWriter).
template <typename PropertyListReaderType, typename PropertyListWriterType>
class FileSettingsStore : public RefCounted {
public:
    FileSettingsStore() { }

    ~FileSettingsStore() { }

    typedef typename PropertyListWriterType::Options PropertyListWriterOptions;

    /// If defaultsPath is NULL, defaults are not loaded and defaultsFileSystem is ignored. The log is retained.
    bool init(FileSystem* fileSystem, const char* path, bool fileMustExist, FileSystem* defaultsFileSystem,
        const char* defaultsPath, const Value::Dictionary& defaultDefaults, Log* log,
        const Value::Dictionary& commandLineSettings,
        const PropertyListWriterOptions& writerOptions = PropertyListWriterOptions())
    {
        _fileSystem = fileSystem;
        _path = path ? path : "";
        _log = log;
        _writerOptions = writerOptions;

        _store.setCommandLine(commandLineSettings);

        if (!StringIsEmpty(defaultsPath) && defaultsFileSystem) {
            _log->trace("Loading default settings: %s", defaultsPath);

            Value settings;
            if (RefPtr<Stream> stream = defaultsFileSystem->openForRead(defaultsPath,
                    DowngradeLog(PrefixLog(log, defaultsPath),
                        Log::LevelWarning))) {
                settings = PropertyListReaderType().read(stream, PrefixLog(log, defaultsPath));
            }

            if (!settings.isUndefined()) {
                _store.setDefaults(settings.getDictionary());
                // _log->trace("Default settings: %s", settings.toFormattedJSON().c_str());

            } else {
                // If we've been told we have a defaults file and we can't load it, abort, whether the file
                // exists or not (either way it's an error).
                _log->error(PRIME_LOCALISE("Couldn't load default settings: %s"), defaultsPath);
                return false;
            }
            _store.setReportMissingSettings(true);

        } else {
            _store.setDefaults(defaultDefaults);
            _store.setReportMissingSettings(false);
        }

        if (_fileSystem && !_path.empty()) {
            _log->trace("Loading settings: %s", _path.c_str());

            Value settings;
            if (RefPtr<Stream> stream = _fileSystem->openForRead(_path.c_str(),
                    TraceLog(PrefixLog(log, _path.c_str())))) {
                settings = PropertyListReaderType().read(stream, PrefixLog(log, _path.c_str()));
                if (settings.isUndefined()) {
                    _log->error(PRIME_LOCALISE("Couldn't load settings: %s"), _path.c_str());
                    return false;
                }

            } else {
                if (fileMustExist) {
                    _log->error(PRIME_LOCALISE("Settings file not found: %s"), _path.c_str());
                    return false;
                }
            }

            if (!settings.isUndefined()) {
                _store.setSettings(settings.getDictionary());
            }
        }

#ifdef PRIME_CXX11_STL
        _store.setFlushCallback([this](DictionarySettingsStore* store, const Value::Dictionary& settings) -> bool {
            return this->flushSettings(store, settings);
        });
#else
        _store.setFlushCallback(MethodCallback(this, &FileSettingsStore::flushSettings));
#endif
        _settings = _store.getSettings();
        return true;
    }

    void setReportMissingSettings(bool reportMissingSettings)
    {
        _store.setReportMissingSettings(reportMissingSettings);
    }

    void setReportAllSettings(bool reportAllSettings)
    {
        _store.setReportAllSettings(reportAllSettings);
    }

    Settings* getSettings() const { return _settings; }

    void flush(bool force = false)
    {
        if (_store.hasFlushCallback()) {
            _store.flushDictionary(force);
        }
    }

    void close()
    {
        if (_store.hasFlushCallback()) {
            _store.flush();
            _store.setFlushCallback(DictionarySettingsStore::FlushCallback());
        }

        _settings.release();
    }

private:
    bool flushSettings(DictionarySettingsStore*, const Value::Dictionary& settings)
    {
        PrefixLog log(_log, _path.c_str());

        _log->trace("Saving settings: %s", _path.c_str());

        RefPtr<Stream> stream = _fileSystem->openForAtomicWrite(_path.c_str(), log);
        if (!stream) {
            return false;
        }

        char buffer[1024];
        if (!PropertyListWriterType().write(stream, log, settings, _writerOptions, sizeof(buffer), buffer) || !stream->close(log)) {
            return false;
        }

        return true;
    }

    DictionarySettingsStore _store;
    RefPtr<Settings> _settings;
    RefPtr<Log> _log;
    RefPtr<FileSystem> _fileSystem;
    std::string _path;
    PropertyListWriterOptions _writerOptions;
};
}

#endif
