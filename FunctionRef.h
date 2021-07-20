// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_FUNCTIONREF_H
#define PRIME_FUNCTIONREF_H

#include "Config.h"
#include <type_traits>
#include <utility>

namespace Prime {

template <typename FunctionSignature>
class FunctionRef;

/// A lightweight reference to a function that's significantly less expensive than a std::function. Since it's
/// only a reference to the function, FunctionRef's must not be copied.
template <typename Return, typename... Args>
class FunctionRef<Return(Args...)> {
public:
    template <typename Callable>
    FunctionRef(Callable&& callable)
        : _callback(CallbackFunction<typename std::remove_reference<Callable>::type>)
        , _callable(reinterpret_cast<void*>(&callable))
    {
    }

    Return operator()(Args... args)
    {
        return _callback(_callable, std::forward<Args>(args)...);
    }

private:
    Return (*_callback)(void* callable, Args... args);
    void* _callable;

    template <typename Callable>
    static Return CallbackFunction(void* callable, Args... args)
    {
        return (*reinterpret_cast<Callable*>(callable))(std::forward<Args>(args)...);
    }
};
}

#endif
