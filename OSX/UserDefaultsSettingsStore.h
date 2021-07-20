// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OSX_USERDEFAULTSSETTINGSSTORE_H
#define PRIME_OSX_USERDEFAULTSSETTINGSSTORE_H

#include "../Settings.h"
#include <Foundation/Foundation.h>

namespace Prime {

class UserDefaultsSettings;

/// Maps NSUserDefaults to a Settings hierarchy.
class PRIME_PUBLIC UserDefaultsSettingsStore : public Settings::Store {
public:
    UserDefaultsSettingsStore();

    virtual ~UserDefaultsSettingsStore();

    /// The override path separator defaults to '.'. It used to be ' ' (more Mac-ish but difficult to use from
    /// the command line), so legacy code will need to call setPathSeparator(' ');
    void setPathSeparator(char ch);

    /// We need an NSString internally.
    void setPathSeparator(NSString* string);

    NSUserDefaults* getUserDefaults() const;

    NSString* getPathSeparator() const { return _pathSeparator; }

    virtual void flush() PRIME_OVERRIDE;

protected:
    virtual RefPtr<Settings> createSettings(Settings* parent, const char* name) PRIME_OVERRIDE;

    friend class UserDefaultsSettings;

    NSString* _pathSeparator;
};
}

#endif
