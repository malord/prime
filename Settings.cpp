// Copyright 2000-2021 Mark H. P. Lord

#include "Settings.h"
#include "ScopedLock.h"
#include "StringUtils.h"

namespace {

using namespace Prime;

class NullSettings : public Settings {
public:
    NullSettings()
        : Settings(NULL, NULL, NULL)
    {
    }

    virtual Value get(const char*) PRIME_OVERRIDE { return undefined; }

    virtual bool set(const char*, const Value&) PRIME_OVERRIDE { return false; }

    virtual bool remove(const char*) PRIME_OVERRIDE { return false; }

    virtual RefPtr<Settings> getSettings(const char*) PRIME_OVERRIDE { return this; }

    virtual void flush() PRIME_OVERRIDE { }

    virtual RefPtr<Settings> getRoot() const PRIME_OVERRIDE { return const_cast<NullSettings*>(this); }
};
}

namespace Prime {

//
// Settings::Observer
//

Settings::Observer::Observer()
{
}

Settings::Observer::Observer(const Observer&)
{
}

Settings::Observer::~Observer()
{
    close();
}

Settings::Observer& Settings::Observer::operator=(const Observer&)
{
    close();
    return *this;
}

bool Settings::Observer::init(Settings* settings, const Callback& callback, bool invokeCallbackNow)
{
    PRIME_ASSERT(callback);
    PRIME_ASSERT(settings);

    close();

    bool success = settings->addObserver(this);

    if (success) {
        _callback = callback;
        _settings = settings;
    }

    if (invokeCallbackNow) {
        callback(settings);
    }

    return success;
}

void Settings::Observer::close()
{
    if (_settings) {
        _settings->removeObserver(this);
        _settings.release();
    }
}

void Settings::Observer::settingChanged(Settings* settings)
{
    // The Settings will be locked when it calls this method.

    if (_callback) {
        _callback(settings);
    }
}

//
// Settings::Store
//

Settings::Store::Store()
    : _root(NULL)
    , _dirty(false)
    , _reportMissingSettings(true)
    , _reportAllSettings(false)
{
    _mutex.init(Log::getGlobal(), "Settings::Store mutex");
}

Settings::Store::~Store()
{
    if (_root) {
        _root->release();
    }
}

RefPtr<Settings> Settings::Store::getSettings() const
{
    RecursiveMutex::ScopedLock selfLock(getMutex());

    RefPtr<Settings> root;

    if (!_root) {
        root = const_cast<Store*>(this)->createSettings(NULL, NULL);
        _root = root;
    } else {
        root = _root;
    }

    return root;
}

//
// Settings
//

Settings* Settings::getNullSettings()
{
    static NullSettings settings;
    return &settings;
}

Settings::Settings(Store* store, Settings* parent, const char* name)
    : _store(store)
    , _parent(parent)
    , _children(&Settings::_childrenLink)
    , _observers(&Observer::_link)
{
    if (name) {
        _name.reset(NewString(name));
    }
}

Settings::Settings(const Settings&)
    : _children(&Settings::_childrenLink)
    , _observers(&Observer::_link)
{
    // This method is private to disable copying.
}

Settings::~Settings()
{
    RecursiveMutex::ScopedLock lockStore(_store ? _store->getMutex() : NULL);

    if (_parent) {
        _parent->removeChild(this);
    } else if (_store) {
        if (_store->_root == this) {
            _store->_root = NULL;
        }
    }
}

const Settings* Settings::getFirstChild() const
{
    return _children.getFirst();
}

const Settings* Settings::getNextChild(const Settings* child) const
{
    return _children.getNext(child);
}

void Settings::removeChild(Settings* child)
{
    // Destructor has locked the mutex for us
    PRIME_DEBUG_ASSERT(_children.contains(child));
    _children.erase(child);
}

RefPtr<Settings> Settings::getSettings(const char* name)
{
    if (!_store) {
        return NULL;
    }

    RecursiveMutex::ScopedLock lockStore(_store->getMutex());

    for (Settings* child = _children.getFirst(); child; child = _children.getNext(child)) {
        if (child->isName(name)) {
            return child;
        }
    }

    RefPtr<Settings> newSettings = getStore()->createSettings(this, name);

    _children.push_back(newSettings);

    return newSettings;
}

bool Settings::addObserver(Observer* observer)
{
    ScopedLock<Settings> lockSelf(this);

    PRIME_DEBUG_ASSERT(!_observers.contains(observer));
    _observers.push_back(observer);

    return true;
}

void Settings::removeObserver(Observer* observer)
{
    ScopedLock<Settings> lockSelf(this);

    PRIME_DEBUG_ASSERT(_observers.contains(observer));
    _observers.erase(observer);
}

void Settings::invokeObservers() const
{
    ScopedLock<const Settings> lockSelf(this);

    invokeObserversWithoutLocking();
}

void Settings::invokeObserversWithoutLocking() const
{
    Settings* mutableThis = const_cast<Settings*>(this);

    for (const Observer* observer = _observers.getFirst(); observer; observer = _observers.getNext(observer)) {
        const_cast<Observer*>(observer)->settingChanged(mutableThis);
    }

    if (Settings* parent = mutableThis->getParent()) {
        parent->invokeObserversWithoutLocking();
    }
}

void Settings::recursivelyInvokeObservers() const
{
    ScopedLock<const Settings> lockSelf(this);

    recursivelyInvokeObserversWithoutLocking();
}

void Settings::recursivelyInvokeObserversWithoutLocking() const
{
    const Settings* next;
    for (const Settings* child = getFirstChild(); child; child = next) {
        RefPtr<const Settings> retainChild(child);
        child->recursivelyInvokeObserversWithoutLocking();
        next = getNextChild(child);
    }

    invokeObserversWithoutLocking();
}

bool Settings::setAndInvokeObservers(const char* name, const Value& value)
{
    bool result = set(name, value);
    invokeObservers();
    return result;
}

bool Settings::removeAndInvokeObservers(const char* name)
{
    bool result = remove(name);
    invokeObservers();
    return result;
}

void Settings::flush()
{
    if (_store) {
        _store->flush();
    }
}

RefPtr<Settings> Settings::getRoot() const
{
    if (_store) {
        return _store->getSettings();
    }

    return NULL;
}

void Settings::lock() const
{
    if (_store) {
        _store->getMutex()->lock();
    }
}

void Settings::unlock() const
{
    if (_store) {
        _store->getMutex()->unlock();
    }
}

bool Settings::isName(const char* name) const
{
    if (!_name.get()) {
        return name == NULL;
    }

    if (!name) {
        return false;
    }

    return StringsEqual(name, _name.get());
}
}
