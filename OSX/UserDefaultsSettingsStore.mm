// Copyright 2000-2021 Mark H. P. Lord

#include "UserDefaultsSettingsStore.h"
#include "../StringUtils.h"
#include "ScopedAutoreleasePool.h"
#include "ValueNSObject.h"

namespace Prime {

//
// UserDefaultsSettings declaration
//

class UserDefaultsSettings : public Settings {
public:
    UserDefaultsSettings(Store* store, Settings* parent, const char* name);

    virtual ~UserDefaultsSettings();

    virtual Value get(const char* name) PRIME_OVERRIDE;

    virtual bool set(const char* name, const Value& value) PRIME_OVERRIDE;

    virtual bool remove(const char* name) PRIME_OVERRIDE;

    UserDefaultsSettingsStore* getUserDefaultsSettingsStore() const { return static_cast<UserDefaultsSettingsStore*>(getStore()); }

protected:
    static const unsigned int maxSettingDepth = 20u;

    /// Fills path (which must have room for maxSettingDepth entries) with the path to a setting within
    /// this Settings, called name. So for the setting "/Editor/CardboardObject/drawShadows", the path
    /// would be { "Editor", "CardboardObject", "DrawShadows", 0 }.
    bool buildPath(const char** path, const char* name) const;

    NSString* buildOverrideName(const char** path) const;

    NSString* buildOverrideName(const char* name) const;
};

//
// UserDefaultsSettings
//

UserDefaultsSettings::UserDefaultsSettings(Store* store, Settings* parent, const char* name)
    : Settings(store, parent, name)
{
}

UserDefaultsSettings::~UserDefaultsSettings()
{
}

Value UserDefaultsSettings::get(const char* name)
{
    ScopedAutoreleasePool pool;

    const char* path[maxSettingDepth];

    if (!buildPath(path, name)) {
        return undefined;
    }

    UserDefaultsSettingsStore* store = getUserDefaultsSettingsStore();

    id defaults = store->getUserDefaults();

    // Look for an override. This is a setting in the NSUserDefaults with the path to the setting separated by
    // the path separator. e.g., "Editor.CardboardObject.drawShadows".

    NSString* overrideName = buildOverrideName(path);
    if (!overrideName) {
        return undefined;
    }

    id overrideValue = [defaults objectForKey:overrideName];
    if (overrideValue) {
        if (store->getReportAllSettings()) {
            Trace("Found setting in overrides: %s", [overrideName UTF8String]);
        }
        return ToValue(overrideValue);
    }

    // Now look for a default that's been registered with NSUserDefaults. This involves scanning a hierarchy of
    // dictionaries.

    id dict = defaults;

    const char** pathPtr = path;

    while (pathPtr[1]) {
        dict = [dict objectForKey:[NSString stringWithUTF8String:*pathPtr]];
        ++pathPtr;
    }

    if (dict) {
        id dictValue = [dict objectForKey:[NSString stringWithUTF8String:*pathPtr]];

        if (dictValue) {
            if (store->getReportAllSettings()) {
                Trace("Found setting in defaults: %s", [overrideName UTF8String]);
            }
            return ToValue(dictValue);
        }
    }

    if (store->getReportMissingSettings() || store->getReportAllSettings()) {
        if (!strstr(name, "__useDefault")) {
            char useDefaultName[128];
            StringCopy(useDefaultName, name);
            StringAppend(useDefaultName, "__useDefault");
            if (!get(useDefaultName).toBool()) {
                Trace("Missing setting: %s", [overrideName UTF8String]);
            } else if (store->getReportAllSettings()) {
                Trace("Using code default for setting: %s", [overrideName UTF8String]);
            }
        }
    }

    return undefined;
}

bool UserDefaultsSettings::buildPath(const char** path, const char* name) const
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
    pathPtr[-1] = NULL;
    return true;
}

NSString* UserDefaultsSettings::buildOverrideName(const char** path) const
{
    NSMutableString* string = [[NSMutableString alloc] initWithCapacity:100];

    for (const char** pathPtr = path; *pathPtr; ++pathPtr) {
        if (pathPtr != path) {
            [string appendString:getUserDefaultsSettingsStore()->getPathSeparator()];
        }

        [string appendString:[NSString stringWithUTF8String:*pathPtr]];
    }

    return [string autorelease];
}

NSString* UserDefaultsSettings::buildOverrideName(const char* name) const
{
    const char* path[maxSettingDepth];

    if (!buildPath(path, name)) {
        return nil;
    }

    return buildOverrideName(path);
}

bool UserDefaultsSettings::set(const char* name, const Value& value)
{
    ScopedAutoreleasePool pool;

    NSString* overrideName = buildOverrideName(name);
    if (!overrideName) {
        return false;
    }

    id object = ToNSObject(value);
    if (!object) {
        return false;
    }

    [getUserDefaultsSettingsStore()->getUserDefaults() setObject:object forKey:overrideName];

    return true;
}

bool UserDefaultsSettings::remove(const char* name)
{
    ScopedAutoreleasePool pool;

    NSString* overrideName = buildOverrideName(name);
    if (!overrideName) {
        return false;
    }

    [getUserDefaultsSettingsStore()->getUserDefaults() removeObjectForKey:overrideName];

    return true;
}

//
// UserDefaultsSettingsStore
//

UserDefaultsSettingsStore::UserDefaultsSettingsStore()
    : _pathSeparator(nil)
{
    setPathSeparator('.');
}

UserDefaultsSettingsStore::~UserDefaultsSettingsStore()
{
    [_pathSeparator release];
}

RefPtr<Settings> UserDefaultsSettingsStore::createSettings(Settings* parent, const char* name)
{
    return PassRef(new UserDefaultsSettings(this, parent, name));
}

NSUserDefaults* UserDefaultsSettingsStore::getUserDefaults() const
{
    return [NSUserDefaults standardUserDefaults];
}

void UserDefaultsSettingsStore::setPathSeparator(char ch)
{
    [_pathSeparator release];
    char utf8[2] = { ch, 0 };
    _pathSeparator = [[NSString stringWithUTF8String:utf8] retain];
}

void UserDefaultsSettingsStore::setPathSeparator(NSString* string)
{
    if (_pathSeparator != string) {
        [_pathSeparator release];
        _pathSeparator = [string copy];
    }
}

void UserDefaultsSettingsStore::flush()
{
    ScopedAutoreleasePool pool;

    if (getUserDefaults()) {
        [getUserDefaults() synchronize];
    }
}
}
