// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WINDOWS_REGISTRYSETTINGSSTORE_H
#define PRIME_WINDOWS_REGISTRYSETTINGSSTORE_H

#include "../Settings.h"
#include "WindowsConfig.h"

namespace Prime {

class RegistrySettings;

// TODO

/// Maps a key in the Windows registry to a Settings hierarchy.
class PRIME_PUBLIC RegistrySettingsStore : public Settings::Store {
public:
    RegistrySettingsStore();

    virtual ~RegistrySettingsStore();

    bool init(HKEY key, LPCTSTR subKey, Log* log);

    bool isInitialised() const { return _key != NULL; }

    virtual void flush() PRIME_OVERRIDE;

protected:
    virtual RefPtr<Settings> createSettings(Settings* parent, const char* name);

    friend class RegistrySettings;

    HKEY _key;
    std::basic_string<TCHAR> _subkey;
};
}

#endif
