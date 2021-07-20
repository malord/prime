// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CACHE_H
#define PRIME_CACHE_H

#include "Config.h"

#ifdef PRIME_CXX11_STL

#include "Clocks.h"
#include "Mutex.h"
#include <functional>
#include <map>

namespace Prime {

//
// Cache
//

template <class Key, class Value>
class Cache {
public:
    /// If it's possible for getter() to fail, use an Optional as the Value template parameter.
    const Value& get(const Key& key, uint32_t maxAge, const std::function<Value(const Key&)>& getter)
    {
        uint32_t now = Clock::getLoopingMonotonicMilliseconds32();

        auto iter = _cache.find(key);
        if (iter != _cache.end()) {
            uint32_t age = now - iter->second.time;
            if (age > maxAge) {
                iter->second.value = std::move(getter(key));
                iter->second.time = now;
            }

            return iter->second.value;
        }

        CacheValue cv;
        cv.value = getter(key);
        cv.time = now;

        auto result = _cache.insert(std::make_pair(key, std::move(cv)));
        return result.first->second.value;
    }

private:
    struct CacheValue {
        Value value;
        uint32_t time;
    };

    std::map<Key, CacheValue> _cache;
};

//
// ThreadSafeCache
//

template <class Key, class Value>
class ThreadSafeCache {
public:
    bool init(Log* log)
    {
        return _mutex.init(log);
    }

    Value get(const Key& key, uint32_t maxAge, const std::function<Value(const Key&)>& getter)
    {
        Mutex::ScopedLock lock(&_mutex);
        Value value(_cache.get(key, maxAge, getter));
        return value;
    }

private:
    Cache<Key, Value> _cache;
    Mutex _mutex;
};

}

#endif // PRIME_CXX11_STL

#endif
