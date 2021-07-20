// Copyright 2000-2021 Mark H. P. Lord

#include "DictionarySettingsStore.h"
#include "StringUtils.h"

namespace Prime {

//
// DictionarySettings declaration
//

class DictionarySettings : public Settings {
public:
    DictionarySettings(Store* store, Settings* parent, const char* name);

    virtual ~DictionarySettings();

    virtual Value get(const char* name) PRIME_OVERRIDE;

    virtual bool set(const char* name, const Value& value) PRIME_OVERRIDE;

    virtual bool remove(const char* name) PRIME_OVERRIDE;

    DictionarySettingsStore* getDictionarySettingsStore() const { return static_cast<DictionarySettingsStore*>(getStore()); }

protected:
    enum { maxSettingDepth = 20u };

    /// Fills path (which must have room for maxSettingDepth entries) with the path to a setting within
    /// this Settings, called name. So for the setting "/Editor/CardboardObject/drawShadows", the path
    /// would be { "Editor", "CardboardObject", "drawShadows", 0 }.
    bool buildPath(const char** path, const char* name) const;

    bool buildOverrideName(std::string& string, const char** path) const;

    bool buildOverrideName(std::string& string, const char* name) const;
};

//
// DictionarySettings
//

DictionarySettings::DictionarySettings(Store* store, Settings* parent, const char* name)
    : Settings(store, parent, name)
{
}

DictionarySettings::~DictionarySettings()
{
}

Value DictionarySettings::get(const char* name)
{
    const char* path[maxSettingDepth] = { NULL };

    if (!buildPath(path, name)) {
        return undefined;
    }

    std::string overrideName;
    if (!buildOverrideName(overrideName, path)) {
        return undefined;
    }

    DictionarySettingsStore* store = getDictionarySettingsStore();

    RecursiveMutex::ScopedLock lockStore(store->getMutex());

    // Look for an override. This is a setting in the Dictionary with the path to the setting separated by
    // dots. e.g., "Editor.CardboardObject.drawShadows".

    Value overrideValue = store->get(overrideName.c_str(), lockStore);
    if (!overrideValue.isUndefined()) {
        if (store->getReportAllSettings()) {
            Trace("Found setting in overrides: %s", overrideName.c_str());
        }
        return overrideValue;
    }

    // Now look for a default that's been registered with Dictionary. This involves scanning a hierarchy of dictionaries.
    const Value::Dictionary* dict = NULL;

    const char** pathPtr = path;

    while (pathPtr[1]) {
        if (pathPtr == path) {
            dict = &store->get(*pathPtr, lockStore).getDictionary();
        } else {
            dict = &dict->get(*pathPtr).getDictionary();
        }

        if (dict->empty()) {
            break;
        }

        ++pathPtr;
    }

    if (dict) {
        const Value& dictValue = dict->get(*pathPtr);

        if (!dictValue.isUndefined()) {
            if (store->getReportAllSettings()) {
                Trace("Found setting in defaults: %s", overrideName.c_str());
            }
            return dictValue;
        }
    }

    if (store->getReportMissingSettings() || store->getReportAllSettings()) {
        if (!strstr(name, "__useDefault")) {
            char useDefaultName[128];
            StringCopy(useDefaultName, sizeof(useDefaultName), name);
            StringAppend(useDefaultName, sizeof(useDefaultName), "__useDefault");
            if (!get(useDefaultName).toBool()) {
                Trace("Missing setting: %s", overrideName.c_str());
            } else if (store->getReportAllSettings()) {
                Trace("Using code default for setting: %s", overrideName.c_str());
            }
        }
    }

    return undefined;
}

bool DictionarySettings::buildPath(const char** path, const char* name) const
{
    const char** pathPtr = path;
    const char** pathEnd = path + maxSettingDepth;

    *pathPtr++ = name;

    for (const Settings* scan = this; scan; scan = scan->getParent()) {
        if (pathPtr == pathEnd) {
            return false;
        }

        *pathPtr++ = scan->getName();
    }

    std::reverse(path, pathPtr - 1);
    return true;
}

bool DictionarySettings::buildOverrideName(std::string& string, const char** path) const
{
    for (const char** pathPtr = path; *pathPtr; ++pathPtr) {
        if (pathPtr != path) {
            string += '.';
        }

        string += *pathPtr;
    }

    return true;
}

bool DictionarySettings::buildOverrideName(std::string& string, const char* name) const
{
    const char* path[maxSettingDepth] = { NULL };

    if (!buildPath(path, name)) {
        return false;
    }

    return buildOverrideName(string, path);
}

bool DictionarySettings::set(const char* name, const Value& value)
{
    std::string overrideName;
    if (!buildOverrideName(overrideName, name)) {
        return false;
    }

    RecursiveMutex::ScopedLock lockStore(getStore()->getMutex());

    getDictionarySettingsStore()->set(overrideName.c_str(), value, lockStore);

    return true;
}

bool DictionarySettings::remove(const char* name)
{
    std::string overrideName;
    if (!buildOverrideName(overrideName, name)) {
        return false;
    }

    RecursiveMutex::ScopedLock lockStore(getStore()->getMutex());

    getDictionarySettingsStore()->remove(overrideName.c_str(), lockStore);

    return true;
}

//
// DictionarySettingsStore
//

DictionarySettingsStore::DictionarySettingsStore()
{
}

DictionarySettingsStore::~DictionarySettingsStore()
{
    _commandLine.clear();
    _settings.clear();
    _defaults.clear();
}

RefPtr<Settings> DictionarySettingsStore::createSettings(Settings* parent, const char* name)
{
    return PassRef(new DictionarySettings(this, parent, name));
}

void DictionarySettingsStore::setSettings(Value::Dictionary settings)
{
    RecursiveMutex::ScopedLock lockStore(getMutex());
    _settings.swap(settings);
    setDirty(false);
}

void DictionarySettingsStore::setCommandLine(Value::Dictionary commandLine)
{
    RecursiveMutex::ScopedLock lockStore(getMutex());
    _commandLine.swap(commandLine);
}

void DictionarySettingsStore::setDefaults(Value::Dictionary defaults)
{
    RecursiveMutex::ScopedLock lockStore(getMutex());
    _defaults.swap(defaults);
}

const Value& DictionarySettingsStore::get(const char* key, ScopedLock& lock) const
{
    PRIME_ASSERT(lock.isLocked());

    Value::Dictionary::const_iterator iter = findOnCommandLine(key, lock);
    if (iter != _commandLine.end()) {
        return iter->second;
    }

    const Value& got = _settings.get(key);
    if (!got.isUndefined()) {
        return got;
    }

    return _defaults.get(key);
}

Value::Dictionary::const_iterator DictionarySettingsStore::findOnCommandLine(const char* key, ScopedLock& lock) const
{
    return const_cast<DictionarySettingsStore*>(this)->findOnCommandLine(key, lock);
}

Value::Dictionary::iterator DictionarySettingsStore::findOnCommandLine(const char* key, ScopedLock& lock)
{
    PRIME_ASSERT(lock.isLocked());
    for (Value::Dictionary::iterator iter = _commandLine.begin(); iter != _commandLine.end(); ++iter) {
        if (ASCIIEqualIgnoringCase(iter->first, key)) {
            return iter;
        }
    }

    return _commandLine.end();
}

void DictionarySettingsStore::set(const char* key, const Value& value, ScopedLock& lock)
{
    PRIME_ASSERT(lock.isLocked());
    _settings.set(key, value);
    setDirty(true);

    removeKeyFromCommandLine(key, lock);
}

void DictionarySettingsStore::removeKeyFromCommandLine(const char* key, ScopedLock& lock)
{
    PRIME_ASSERT(lock.isLocked());
    // Remove the key from the command line dictionary, so we pick up the new value.
    Value::Dictionary::iterator iter = findOnCommandLine(key, lock);
    if (iter != _commandLine.end()) {
        _commandLine.erase(iter);
    }
}

void DictionarySettingsStore::remove(const char* key, ScopedLock& lock)
{
    PRIME_ASSERT(lock.isLocked());
    if (_settings.erase(key)) {
        setDirty(true);
    }

    removeKeyFromCommandLine(key, lock);
}

void DictionarySettingsStore::flush()
{
    flushDictionary(false);
}

void DictionarySettingsStore::flushDictionary(bool force)
{
    if (_flushCallback) {
        RecursiveMutex::ScopedLock lockStore(getMutex());

        if (isDirty() || force) {
            if (_flushCallback(this, _settings)) {
                setDirty(false);
            }
        }
    }
}
}
