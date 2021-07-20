// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TASKSYSTEMSELECTOR_H
#define PRIME_TASKSYSTEMSELECTOR_H

#include "Config.h"

#if !defined(PRIME_NO_GCD)
#include "OSX/GCDConfig.h"
#if !defined(PRIME_NO_GCD)
#include "OSX/GCDTaskSystem.h"
#endif
#endif

#include "ScopedPtr.h"
#include "StringUtils.h"
#include "ThreadPoolTaskSystem.h"

#include <string.h>

namespace Prime {

/// Use this instead of a Global<DefaultTaskSystem> where you want the task system to be a runtime option.
class PRIME_PUBLIC TaskSystemSelector {
public:
    explicit TaskSystemSelector(bool becomeGlobal = true)
        : _name(NULL)
        , _becomeGlobal(becomeGlobal)
    {
    }

    ~TaskSystemSelector()
    {
        if (_threadPool) {
            _threadPool->close();
        }
        _threadPoolSystem.reset();
        if (_threadPool) {
            ThreadPool* tp = _threadPool.detach();
            if (tp->release() != 0) {
                DeveloperWarning("ThreadPool still had references.");
            }
        }
    }

    /// If not called, or called with name == NULL, the default task system for the platform is used.
    void select(const char* name) { _name = name; }

    bool init(int concurrentThreadCount, int maxThreadCount, size_t stackSize, Log* log)
    {
#if !defined(PRIME_NO_GCD)
        if (!_name || !*_name || StringsEqual(_name, "gcd")) {
            _threadPoolSystem.reset();
            return initGCD(concurrentThreadCount, maxThreadCount, stackSize, log);
        }

        _gcdSystem.reset();
#endif

        if (!_name || !*_name || StringsEqual(_name, "threadpool") || StringsEqual(_name, "tp")) {
            return initThreadPool(concurrentThreadCount, maxThreadCount, stackSize, log);
        }

        _threadPoolSystem.reset();

        log->error(PRIME_LOCALISE("Unknown task system: %s"), _name);
        return false;
    }

private:
#if !defined(PRIME_NO_GCD)
    bool initGCD(int concurrentThreadCount, int maxThreadCount, size_t stackSize, Log* log)
    {
        _gcdSystem.reset(new GCDTaskSystem);
        if (_becomeGlobal) {
            TaskSystem::setGlobal(_gcdSystem.get());
        }
        return _gcdSystem->init(concurrentThreadCount, maxThreadCount, stackSize, log);
    }
#endif

    bool initThreadPool(int concurrentThreadCount, int maxThreadCount, size_t stackSize, Log* log)
    {
        _threadPoolSystem.reset(new ThreadPoolTaskSystem);
        if (_becomeGlobal) {
            TaskSystem::setGlobal(_threadPoolSystem.get());
        }

        _threadPool = PassRef(new ThreadPool);
        if (!_threadPool->init(concurrentThreadCount, maxThreadCount, stackSize, log, "Global thread pool")) {
            return false;
        }

        return _threadPoolSystem->init(_threadPool, log);
    }

    ScopedPtr<ThreadPoolTaskSystem> _threadPoolSystem;
    RefPtr<ThreadPool> _threadPool;

#if !defined(PRIME_NO_GCD)
    ScopedPtr<GCDTaskSystem> _gcdSystem;
#endif

    const char* _name;
    bool _becomeGlobal;
};
}

#endif
