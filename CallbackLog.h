// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CALLBACKLOG_H
#define PRIME_CALLBACKLOG_H

#include "Mutex.h"
#include "TextLog.h"
#ifndef PRIME_CXX11_STL
#include "Callback.h"
#endif
#include <functional>

namespace Prime {

/// A TextLog that invokes a callback (or a std::function since C++11).
class CallbackLog : public TextLog {
public:
#ifdef PRIME_CXX11_STL
    typedef std::function<void(const char*)> Callback;
    typedef std::function<void(Log::Level, const char*)> LevelCallback;
#else
    typedef Callback1<void, const char*> Callback;
    typedef Callback2<void, Log::Level, const char*> LevelCallback;
#endif

    CallbackLog()
    {
    }

    explicit CallbackLog(const Callback& callback) { setCallback(callback); }

    explicit CallbackLog(const LevelCallback& callback) { setCallback(callback); }

    void setCallback(const Callback& callback);

    void setCallback(const LevelCallback& callback);

    void clearCallback();

protected:
    virtual void write(Level level, const char* string) PRIME_OVERRIDE;

private:
    void createMutex();

    Callback _callback;
    LevelCallback _levelCallback;
    RecursiveMutex _mutex;
};
}

#endif
