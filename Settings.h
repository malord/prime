// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_SETTINGS_H
#define PRIME_SETTINGS_H

#include "DoubleLinkList.h"
#include "Value.h"
#ifndef PRIME_CXX11_STL
#include "Callback.h"
#endif
#include "Mutex.h"
#include "ScopedPtr.h"
#include <functional>

namespace Prime {

/// An object capable of retrieving and storing settings. Settings form a hierarchy, so you can, for example, do:
/// `mySettings->getSettings("Render")->get("fullscreen").toBool(true)`. You can also retain() a Settings from
/// within the hierarchy for later access.
class PRIME_PUBLIC Settings : public RefCounted {
public:
    /// Returns a Settings instance where reads return nothing and writes are silently discarded.
    static Settings* getNullSettings();

    /// Observes changes to a Settings node. You'd usually construct an Observer as a member variable of the
    /// object that needs to observe changes so that it automatically stops observing when destructed.
    /// There used to be a whole bunch of code in here for cvars, but I dropped that in favour of a simple
    /// callback triggered by invokeObservers() or recursivelyInvokeObservers(), allowing groups of settings
    /// changes to be implemented more efficiently.
    class PRIME_PUBLIC Observer {
    public:
        Observer();

        ~Observer();

        /// Observers aren't copyable, but don't preclude containing classes from being copyable.
        Observer(const Observer& copy);
        Observer& operator=(const Observer&);

#ifdef PRIME_CXX11_STL
        typedef std::function<void(Settings*)> Callback;
#else
        typedef Callback1<void, Settings*> Callback;
#endif

        /// Begin observing the specified settings instance, assigning a callback to be invoked when any settings
        /// are changed. If invokeCallbackNow is true, the callback is invoked. Returns true on success, false if
        /// settings observation is disabled for the supplied Settings. Even if false is returned, if
        /// invokeCallbackNow is true, the callback will be invoked.
        bool init(Settings* settings, const Callback& callback, bool invokeCallbackNow = true);

        void close();

        /// Returns true if we're installed as a settings observer. Note that the Settings may invoke
        /// close(), e.g., if the store is being destructed.
        bool isInitialised() const { return !_settings.isNull(); }

        /// Invoked by the Settings object, which will be locked at the time of the call.
        void settingChanged(Settings* settings);

    private:
        RefPtr<Settings> _settings;
        DoubleLink<Observer> _link;
        Callback _callback;

        friend class Settings;
    };

    /// Base class for objects which manage a hierarchy of Settings.
    class PRIME_PUBLIC Store : public RefCounted {
    public:
        Store();

        virtual ~Store();

        RecursiveMutex* getMutex() const { return &_mutex; }

        void setReportMissingSettings(bool value) { _reportMissingSettings = value; }
        bool getReportMissingSettings() const { return _reportMissingSettings; }

        void setReportAllSettings(bool value) { _reportAllSettings = value; }
        bool getReportAllSettings() const { return _reportAllSettings; }

        RefPtr<Settings> getSettings() const;

        virtual void flush() = 0;

    protected:
        virtual RefPtr<Settings> createSettings(Settings* parent, const char* name) = 0;

        bool isDirty() const { return _dirty; }

        void setDirty(bool value) { _dirty = value; }

    private:
        mutable RecursiveMutex _mutex;

        bool _reportMissingSettings;
        bool _reportAllSettings;

        mutable Settings* _root;
        bool _dirty;

        friend class Settings;

        PRIME_UNCOPYABLE(Store);
    };

    //
    // Settings
    //

    virtual ~Settings();

    /// Returns an invalid Value if the setting doesn't exist, so you can do: get("bpp").toInt(32) and
    /// will get 32 if the setting is not present. You cannot read hierarchical settings using the dot separator
    /// and must use getSettings().
    virtual Value get(const char* name) = 0;

    /// Storing settings is not guaranteed to be available.
    virtual bool set(const char* name, const Value& value) = 0;

    /// Call set() then call invokeObservers().
    bool setAndInvokeObservers(const char* name, const Value& value);

    /// Removing settings is not guaranteed to be available. If not, the setting will be set to a blank string
    /// and false will be returned.
    virtual bool remove(const char* name) = 0;

    /// Call remove() then call invokeObservers().
    bool removeAndInvokeObservers(const char* name);

    /// Returns a child Settings node. You can't do, e.g., get("A.B.C"), you must do:
    /// getSettings("A")->getSettings("B")->get("C");
    virtual RefPtr<Settings> getSettings(const char* name);

    /// Allows you to do getPath("A.B.C");
    // TODO? Value getPath(StringView path);

    /// Returns this Settings' name within its parent. Returns a null pointer for the root. By calling this on all
    /// Settings instances up to the root you can determine the Settings' path.
    const char* getName() const { return _name.get(); }

    const Settings* getParent() const { return _parent; }
    Settings* getParent() { return _parent; }

    bool addObserver(Observer* observer);

    void removeObserver(Observer* observer);

    /// Invoke all observers of this Settings path.
    void invokeObservers() const;

    /// Invoke all observers of this Settings path and all the child settings it contains.
    void recursivelyInvokeObservers() const;

    virtual void flush();

    /// Must never return a null pointer.
    virtual RefPtr<Settings> getRoot() const;

    /// Lock all the Settings in this hierarchy.
    void lock() const;

    /// Unlock a lock().
    void unlock() const;

protected:
    Settings(Store* store, Settings* parent, const char* name);

    Store* getStore() const { return _store; }

    const Settings* getFirstChild() const;
    Settings* getFirstChild()
    {
        return const_cast<Settings*>(const_cast<const Settings*>(this)->getFirstChild());
    }

    const Settings* getNextChild(const Settings* child) const;
    Settings* getNextChild(Settings* child)
    {
        return const_cast<Settings*>(const_cast<const Settings*>(this)->getNextChild(child));
    }

    void removeChild(Settings* child);

    bool isName(const char* name) const;

private:
    void invokeObserversWithoutLocking() const;
    void recursivelyInvokeObserversWithoutLocking() const;

    ScopedArrayPtr<char> _name;
    RefPtr<Store> _store;
    RefPtr<Settings> _parent;
    DoubleLinkList<Settings, DetachingLinkListElementManager<Settings>> _children;
    DoubleLink<Settings> _childrenLink;

    DoubleLinkList<Observer, DetachingLinkListElementManager<Observer>> _observers;

    Settings(const Settings&);
    Settings& operator=(const Settings&) { return *this; }

    friend class Store;
};
}

#endif
