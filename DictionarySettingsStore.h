// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DICTIONARYSETTINGSSTORE_H
#define PRIME_DICTIONARYSETTINGSSTORE_H

#include "Settings.h"

namespace Prime {

class DictionarySettings;

/// Provides a Settings backed by a Dictionary.
class PRIME_PUBLIC DictionarySettingsStore : public Settings::Store {
public:
    DictionarySettingsStore();

    virtual ~DictionarySettingsStore();

    /// If a setting isn't found on the command line or the persistent settings, we'll look here.
    void setDefaults(Value::Dictionary defaults);

    /// No longer requires the command line keys to be lowercased.
    void setCommandLine(Value::Dictionary commandLine);

    /// Designed to be called only at startup. Calling it any other time won't update any observers.
    void setSettings(Value::Dictionary settings);

/// The callback is invoked with this store locked. Return true if the settings were successfully saved.
#ifdef PRIME_CXX11_STL
    typedef std::function<bool(DictionarySettingsStore*, const Value::Dictionary&)> FlushCallback;
#else
    typedef Callback2<bool, DictionarySettingsStore*, const Value::Dictionary&> FlushCallback;
#endif

    void setFlushCallback(const FlushCallback& value)
    {
        _flushCallback = value;
    }

    bool hasFlushCallback() const { return static_cast<bool>(_flushCallback); }

    virtual void flush() PRIME_OVERRIDE;

    /// Handles locking.
    void flushDictionary(bool force);

    //
    // Direct access to the dictionaries. These are not thread safe.
    //

    Value::Dictionary& getCommandLineDictionary() { return _commandLine; }
    const Value::Dictionary& getCommandLineDictionary() const { return _commandLine; }

    Value::Dictionary& getSettingsDictionary() { return _settings; }
    const Value::Dictionary& getSettingsDictionary() const { return _settings; }

    Value::Dictionary& getDefaultsDictionary() { return _defaults; }
    const Value::Dictionary& getDefaultsDictionary() const { return _defaults; }

protected:
    virtual RefPtr<Settings> createSettings(Settings* parent, const char* name) PRIME_OVERRIDE;

    typedef RecursiveMutex::ScopedLock ScopedLock;

    const Value& get(const char* key, ScopedLock& lock) const;

    void set(const char* key, const Value& value, ScopedLock& lock);

    void remove(const char* key, ScopedLock& lock);

    void removeKeyFromCommandLine(const char* key, ScopedLock& lock);

    Value::Dictionary::const_iterator findOnCommandLine(const char* key, ScopedLock& lock) const;
    Value::Dictionary::iterator findOnCommandLine(const char* key, ScopedLock& lock);

    friend class DictionarySettings;

    Value::Dictionary _commandLine;
    Value::Dictionary _settings;
    Value::Dictionary _defaults;
    FlushCallback _flushCallback;
};
}

#endif
