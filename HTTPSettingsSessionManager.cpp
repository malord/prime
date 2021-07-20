// Copyright 2000-2021 Mark H. P. Lord

#include "HTTPSettingsSessionManager.h"
#include "Clocks.h"
#include "Mutex.h"

namespace Prime {

//
// HTTPSettingsSessionManager::Session
//

class HTTPSettingsSessionManager::MemorySession : public HTTPServer::Session {
public:
    MemorySession(HTTPSettingsSessionManager* sessionManager, std::string sessionID);

    MemorySession(HTTPSettingsSessionManager* sessionManager, std::string sessionID, const Value& propertyList);

    Value saveWhenLocked() const;

    virtual const char* getID() const PRIME_OVERRIDE;

    virtual Value get(const char* key) const PRIME_OVERRIDE;

    virtual void set(const char* key, const Value& value) PRIME_OVERRIDE;

    virtual void remove(const char* key) PRIME_OVERRIDE;

    virtual Value getAndRemove(const char* key) PRIME_OVERRIDE;

    virtual Value::Dictionary toDictionary() const PRIME_OVERRIDE;

    void touch(const UnixTime& unixTime)
    {
        if (unixTime.getSeconds() > _lastAccess) {
            _lastAccess = unixTime.getSeconds();
        }
    }

    const UnixTime getLastAccess() const { return UnixTime(_lastAccess, 0); }

private:
    void construct();

    ReadWriteLock* getLock() const { return &_sessionManager->_lock; }

    HTTPSettingsSessionManager* _sessionManager;
    std::string _id;
    Value::Dictionary _dictionary;
    volatile int64_t _lastAccess;
};

HTTPSettingsSessionManager::MemorySession::MemorySession(HTTPSettingsSessionManager* sessionManager,
    std::string sessionID)
    : _sessionManager(sessionManager)
    , _id(PRIME_MOVE(sessionID))
{
    construct();
}

HTTPSettingsSessionManager::MemorySession::MemorySession(HTTPSettingsSessionManager* sessionManager,
    std::string sessionID, const Value& propertyList)
    : _sessionManager(sessionManager)
    , _id(PRIME_MOVE(sessionID))
{
    construct();
    _dictionary = propertyList.getDictionary();
    _lastAccess = _dictionary["lastAccess"].toInteger(0);
}

void HTTPSettingsSessionManager::MemorySession::construct()
{
    _lastAccess = 0;
}

Value HTTPSettingsSessionManager::MemorySession::saveWhenLocked() const
{
    Value::Dictionary dict;
    dict.reserve(_dictionary.size() + 1);
    dict = _dictionary;
    dict.set("lastAccess", _lastAccess);
    return dict;
}

const char* HTTPSettingsSessionManager::MemorySession::getID() const
{
    return _id.c_str();
}

Value HTTPSettingsSessionManager::MemorySession::get(const char* key) const
{
    ReadWriteLock::ScopedReadLock lock(getLock());
    return _dictionary[key];
}

Value::Dictionary HTTPSettingsSessionManager::MemorySession::toDictionary() const
{
    ReadWriteLock::ScopedReadLock lock(getLock());
    return _dictionary;
}

void HTTPSettingsSessionManager::MemorySession::set(const char* key, const Value& value)
{
    ReadWriteLock::ScopedWriteLock lock(getLock());
    _dictionary.set(key, value);
}

void HTTPSettingsSessionManager::MemorySession::remove(const char* key)
{
    ReadWriteLock::ScopedWriteLock lock(getLock());
    _dictionary.erase(key);
}

Value HTTPSettingsSessionManager::MemorySession::getAndRemove(const char* key)
{
    Value value;

    Value::Dictionary::iterator pair = _dictionary.find(key);
    if (pair == _dictionary.end()) {
        return value;
    }

    value.move(pair->second);
    _dictionary.erase(pair);

    return value;
}

//
// HTTPSettingsSessionManager
//

namespace {
    const char cookieName[] = "SID";
}

PRIME_DEFINE_UID_CAST(HTTPSettingsSessionManager)

HTTPSettingsSessionManager::HTTPSettingsSessionManager()
{
    _lock.init(Log::getGlobal(), "HTTPSettingsSessionManager lock");

    _saveIntervalSeconds = 60; // TODO: make this a setting
    _lastSaveTime = 0;
    _sessionExpirySeconds = 48 * 60 * 60;
}

HTTPSettingsSessionManager::~HTTPSettingsSessionManager()
{
}

RefPtr<HTTPServer::Session> HTTPSettingsSessionManager::createTemporarySession(Log* log)
{
    char sessionID[25];
    generateSessionID(sessionID, sizeof(sessionID) - 1, log);
    sessionID[sizeof(sessionID) - 1] = 0;

    return PassRef(new MemorySession(this, sessionID));
}

RefPtr<HTTPServer::Session> HTTPSettingsSessionManager::getSessionByID(StringView sessionID)
{
    ReadWriteLock::ScopedReadLock lock(&_lock);
    ReadWriteLock::ScopedWriteLock writeLock;

    // TODO: use C++14 heterogeneous lookup
    SessionMap::const_iterator iter = _sessionMap.find(sessionID.to_string());
    if (iter != _sessionMap.end()) {
        return static_cast<MemorySession*>(iter->second.get());
    }

    return nullptr;
}

RefPtr<HTTPServer::Session> HTTPSettingsSessionManager::getSession(HTTPServer::Request& request,
    HTTPServer::Response* response, bool create)
{
    if (HTTPServer::Session* cached = request.getSession()) {
        return cached;
    }

    // Acquire an existing session.
    {
        ReadWriteLock::ScopedReadLock lock(&_lock);
        ReadWriteLock::ScopedWriteLock writeLock;

        flushIfEnoughtTimeHasPassed(&lock, &writeLock, request.getLog());

        std::string sessionID = request.getCookie(cookieName);

        if (!sessionID.empty()) {
            SessionMap::const_iterator iter = _sessionMap.find(sessionID);
            if (iter != _sessionMap.end()) {
                MemorySession* session = static_cast<MemorySession*>(iter->second.get());
                request.setSession(session);
                session->touch(request.getTime());

                return session;
            }
        }
    }

    // Create a session.
    if (create && PRIME_GUARDMSG(response, "need a Response object if create enabled")) {
        ReadWriteLock::ScopedWriteLock lock(&_lock);

        for (;;) {
            char sessionID[25];
            generateSessionID(sessionID, sizeof(sessionID) - 1, request.getLog());
            sessionID[sizeof(sessionID) - 1] = 0;

            SessionMap::iterator iter = _sessionMap.find(sessionID);
            if (iter != _sessionMap.end()) {
                continue;
            }

            std::string sessionIDString(sessionID);

            RefPtr<MemorySession> session = PassRef(new MemorySession(this, sessionIDString));
            session->touch(request.getTime());

            _sessionMap[sessionIDString] = session;

            request.setSession(session.get());

            request.getLog()->trace("Created session: %s", sessionIDString.c_str());

            response->setCookie(MakeString(cookieName, "=", sessionIDString, "; Path=/; HTTPOnly"));

            _lastSaveTime = Clock::getCurrentTime().getSeconds() - _saveIntervalSeconds + 5;

            flushIfEnoughtTimeHasPassed(NULL, &lock, request.getLog());

            return session;
        }
    }

    return NULL;
}

void HTTPSettingsSessionManager::deleteSession(HTTPServer::Request& request, HTTPServer::Response& response)
{
    std::string sessionID = request.getCookie(cookieName);

    if (sessionID.empty()) {
        return;
    }

    {
        ReadWriteLock::ScopedWriteLock lock(&_lock);

        SessionMap::iterator iter = _sessionMap.find(sessionID);
        if (iter != _sessionMap.end()) {
            _sessionMap.erase(iter);
        }
    }

    response.setCookie(MakeString(cookieName, "=0; Path=/; HTTPOnly"));
}

Value HTTPSettingsSessionManager::saveWhenLocked(ReadWriteLock::ScopedWriteLock& lock) const
{
    PRIME_ASSERT(lock.isLocked());
    (void)lock;

    Value::Dictionary dict;

    for (SessionMap::const_iterator iter = _sessionMap.begin(); iter != _sessionMap.end(); ++iter) {
        dict.set(iter->first, static_cast<MemorySession*>(iter->second.get())->saveWhenLocked());
    }

    return dict;
}

void HTTPSettingsSessionManager::setSettings(Settings* settings)
{
    ReadWriteLock::ScopedWriteLock lock(&_lock);

    loadWhenLocked(lock, settings->get("sessions").getDictionary());

    _settings = settings;
}

void HTTPSettingsSessionManager::loadWhenLocked(ReadWriteLock::ScopedWriteLock& lock, const Value::Dictionary& dict)
{
    PRIME_ASSERT(lock.isLocked());
    (void)lock;

    int64_t time = Clock::getCurrentTime().getSeconds();

    _sessionMap.clear();
    for (size_t i = 0; i != dict.size(); ++i) {
        const Value::Dictionary::value_type& pair = dict.pair(i);
        std::string sessionID = pair.first;
        const Value::Dictionary& sessionDictionary = pair.second.getDictionary();
        int64_t lastAccess = sessionDictionary["lastAccess"].toInteger(0);
        if (time - lastAccess < _sessionExpirySeconds && lastAccess < time) {
            RefPtr<MemorySession> session = PassRef(new MemorySession(this, sessionID, Value(sessionDictionary)));
            _sessionMap[sessionID] = session;
        }
    }
}

void HTTPSettingsSessionManager::flush(Log* log)
{
    ReadWriteLock::ScopedWriteLock lock(&_lock); // Writing to _lastSaveTime

    flushWhenLocked(lock, log);
}

void HTTPSettingsSessionManager::flushWhenLocked(ReadWriteLock::ScopedWriteLock& lock, Log* log)
{
    PRIME_ASSERT(lock.isLocked());
    (void)log;
    (void)lock;

    if (_settings) {
        _settings->set("sessions", saveWhenLocked(lock));
        _settings->flush();
        _lastSaveTime = Clock::getCurrentTime().getSeconds();
    }
}

void HTTPSettingsSessionManager::flushIfEnoughtTimeHasPassed(Log* log)
{
    ReadWriteLock::ScopedReadLock lock(&_lock);
    ReadWriteLock::ScopedWriteLock writeLock;

    flushIfEnoughtTimeHasPassed(&lock, &writeLock, log);
}

void HTTPSettingsSessionManager::flushIfEnoughtTimeHasPassed(ReadWriteLock::ScopedReadLock* readLock,
    ReadWriteLock::ScopedWriteLock* writeLock, Log* log)
{
    PRIME_ASSERT((readLock && readLock->isLocked()) || writeLock->isLocked());

    int64_t time = Clock::getCurrentTime().getSeconds();

    if (!_settings) {
        return;
    }

    if (time - _lastSaveTime < _saveIntervalSeconds) {
        return;
    }

    if (readLock) {
        readLock->unlock();
        writeLock->lock(&_lock);
    }

    if (time - _lastSaveTime < _saveIntervalSeconds) {
        return; // Another thread saved.
    }

    flushWhenLocked(*writeLock, log);
}
}
