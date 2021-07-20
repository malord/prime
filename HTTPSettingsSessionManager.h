// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_HTTPSETTINGSSESSIONMANAGER_H
#define PRIME_HTTPSETTINGSSESSIONMANAGER_H

#include "HTTPServer.h"
#include "ReadWriteLock.h"
#include "Settings.h"
#include <map>

namespace Prime {

// (This could easily be refactored in to a HTTPDictionarySettingsManager which required a subclass to
// implement saving)

/// An HTTPServer::SessionManager that stores session contents in memory and persists them via a Settings.
class PRIME_PUBLIC HTTPSettingsSessionManager : public HTTPServer::SessionManager {
    PRIME_DECLARE_UID_CAST(HTTPServer::SessionManager, 0xae34f56f, 0x79c84835, 0x85f86326, 0xf6d322bd)

public:
    HTTPSettingsSessionManager();

    virtual ~HTTPSettingsSessionManager();

    virtual RefPtr<HTTPServer::Session> getSession(HTTPServer::Request& request,
        HTTPServer::Response* response, bool create) PRIME_OVERRIDE;

    virtual RefPtr<HTTPServer::Session> getSessionByID(StringView sessionID) PRIME_OVERRIDE;

    virtual RefPtr<HTTPServer::Session> createTemporarySession(Log* log) PRIME_OVERRIDE;

    virtual void deleteSession(HTTPServer::Request& request, HTTPServer::Response& response) PRIME_OVERRIDE;

    /// The session list can be persisted in settings. When this is called, the sessions are loaded.
    void setSettings(Settings* settings);

    virtual void flush(Log* log) PRIME_OVERRIDE;

    void flushIfEnoughtTimeHasPassed(Log* log);

private:
    Value saveWhenLocked(ReadWriteLock::ScopedWriteLock&) const;

    void loadWhenLocked(ReadWriteLock::ScopedWriteLock&, const Value::Dictionary& dict);

    void flushIfEnoughtTimeHasPassed(ReadWriteLock::ScopedReadLock* readLock,
        ReadWriteLock::ScopedWriteLock* writeLock, Log* log);

    void flushWhenLocked(ReadWriteLock::ScopedWriteLock&, Log* log);

    class MemorySession;
    friend class MemorySession;

    mutable ReadWriteLock _lock;

    typedef std::map<std::string, RefPtr<HTTPServer::Session>, StringView::Less> SessionMap;
    SessionMap _sessionMap;

    RefPtr<Settings> _settings;

    int64_t _lastSaveTime;

    int64_t _saveIntervalSeconds;

    int64_t _sessionExpirySeconds;
};
}

#endif
