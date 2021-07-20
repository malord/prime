// Copyright 2000-2021 Mark H. P. Lord

// TODO

#include "RegistrySettingsStore.h"

namespace Prime {

//
// RegistrySettings declaration
//

class RegistrySettings : public Settings {
public:
    RegistrySettings(Store* store, Settings* parent, const char* name);

    virtual ~RegistrySettings();

    virtual Value get(const char* name) PRIME_OVERRIDE;

    virtual bool set(const char* name, const Value& value) PRIME_OVERRIDE;

    virtual bool remove(const char* name) PRIME_OVERRIDE;

    RegistrySettingsStore* getRegistrySettingsStore() const { return static_cast<RegistrySettingsStore*>(getStore()); }
};

//
// RegistrySettings
//

RegistrySettings::RegistrySettings(Store* store, Settings* parent, const char* name)
    : Settings(store, parent, name)
{
}

RegistrySettings::~RegistrySettings()
{
}

Value RegistrySettings::get(const char* name)
{
    // TODO
    PRIME_ONCE(DeveloperWarning("RegistrySettings TODO"));
    return undefined;
}

bool RegistrySettings::set(const char* name, const Value& value)
{
    // TODO
    PRIME_ONCE(DeveloperWarning("RegistrySettings TODO"));
    return false;
}

bool RegistrySettings::remove(const char* name)
{
    // TODO
    PRIME_ONCE(DeveloperWarning("RegistrySettings TODO"));
    return false;
}

//
// RegistrySettingsStore
//

RegistrySettingsStore::RegistrySettingsStore()
    : _key(NULL)
{
}

RegistrySettingsStore::~RegistrySettingsStore()
{
}

bool RegistrySettingsStore::init(HKEY key, LPCTSTR subKey, Log* log)
{
    PRIME_ASSERT(!isInitialised());

    _key = key;
    _subkey = subKey;

    HKEY newKey;
    DWORD err = RegCreateKeyEx(_key, _subkey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &newKey, NULL);
    if (err != ERROR_SUCCESS) {
        log->logWindowsError(err, "RegCreateKeyEx");
        return false;
    }

    RegCloseKey(newKey);
    return true;
}

RefPtr<Settings> RegistrySettingsStore::createSettings(Settings* parent, const char* name)
{
    return PassRef(new RegistrySettings(this, parent, name));
}

void RegistrySettingsStore::flush()
{
}
}
