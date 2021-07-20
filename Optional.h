// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OPTIONAL_H
#define PRIME_OPTIONAL_H

#include "Config.h"

namespace Prime {

// This is not strictly safe enough to be considered compatible with std::nullopt.
enum NullOptional {
    nullopt
};

#ifdef PRIME_CXX11

/// Emulate std::optional. May or may not contain an object of type Type. Use operator bool to test.
template <typename Type>
class Optional {
public:
    // TODO: emplace, and emplace constructors

    Optional()
        : _none(true)
    {
    }

    Optional(NullOptional)
        : _none(true)
    {
    }

    ~Optional()
    {
        clear();
    }

    Optional& operator=(NullOptional)
    {
        clear();
        return *this;
    }

    Optional(const Type& copy)
        : _none(false)
    {
        new (&_memory.memory) Type(copy);
    }

    Optional& operator=(const Type& copy)
    {
        if (_none) {
            _none = false;
            new (&_memory.memory) Type(copy);
        } else {
            unsafe_value() = copy;
        }
        return *this;
    }

    Optional(Type&& rvalue) throw()
        : _none(false)
    {
        new (&_memory.memory) Type(std::move(rvalue));
    }

    Optional& operator=(Type&& rvalue) throw()
    {
        if (_none) {
            _none = false;
            new (&_memory.memory) Type(std::move(rvalue));
        } else {
            unsafe_value() = std::move(rvalue);
        }
        return *this;
    }

    Optional(const Optional& copy)
        : _none(copy._none)
    {
        if (!_none) {
            new (&_memory.memory) Type(copy.unsafe_value());
        }
    }

    Optional& operator=(const Optional& copy)
    {
        if (copy._none) {
            clear();
        } else {
            operator=(copy.value());
        }
        return *this;
    }

    Optional(Optional&& rvalue) throw()
        : _none(rvalue._none)
    {
        if (!_none) {
            new (&_memory.memory) Type(std::move(rvalue.unsafe_value()));
            rvalue.clear();
        }
    }

    Optional& operator=(Optional&& rvalue) throw()
    {
        if (rvalue._none) {
            clear();
        } else {
            operator=(std::move(rvalue.unsafe_value()));
            rvalue.clear();
        }
        return *this;
    }

    Type& value()
    {
        PRIME_DEBUG_ASSERT(!_none);
        return unsafe_value();
    }

    const Type& value() const
    {
        PRIME_DEBUG_ASSERT(!_none);
        return unsafe_value();
    }

    Type value_or(const Type& or_value) const
    {
        return _none ? or_value : unsafe_value();
    }

    Type value_or(Type&& or_value) const
    {
        return _none ? std::move(or_value) : unsafe_value();
    }

    Type* operator->()
    {
        return &value();
    }

    const Type* operator->() const
    {
        return &value();
    }

    Type& operator*() &
    {
        return value();
    }

    const Type& operator*() const&
    {
        return value();
    }

    const Type&& operator*() const&&
    {
        return std::move(value());
    }

    Type&& operator*() &&
    {
        return std::move(value());
    }

    bool operator!() const { return _none; }

#ifdef PRIME_COMPILER_EXPLICIT_CONVERSION
    explicit operator bool() const
    {
        return !_none;
    }
#endif

    bool has_value() const
    {
        return !_none;
    }

    bool operator==(const Optional& rhs) const
    {
        if (_none) {
            return rhs._none;
        } else if (rhs._none) {
            return false;
        }
        return unsafe_value() == rhs.unsafe_value();
    }

    bool operator<(const Optional& rhs) const
    {
        if (_none) {
            return !rhs._none;
        } else if (rhs._none) {
            return false;
        }
        return unsafe_value() < rhs.unsafe_value();
    }

    //PRIME_IMPLIED_COMPARISONS_OPERATORS(const Optional&)

    bool operator==(NullOptional) const
    {
        return _none;
    }

    bool operator!=(NullOptional) const
    {
        return !_none;
    }

    void swap(Optional& with)
    {
        if (_none) {
            if (with._none) {
                return;
            }

            new (&_memory.memory) Type(std::move(with.unsafe_value()));
            _none = false;
            with.clear();
            return;
        }

        if (with._none) {
            new (&with._memory.memory) Type(std::move(unsafe_value()));
            with._none = false;
            clear();
            return;
        }

        Type temp(std::move(unsafe_value()));
        unsafe_value() = std::move(with.unsafe_value());
        with.unsafe_value() = std::move(temp);
    }

private:
    void clear()
    {
        if (!_none) {
            unsafe_value().~Type();
            _none = true;
        }
    }

    Type& unsafe_value()
    {
        return *reinterpret_cast<Type*>(&_memory.memory);
    }

    const Type& unsafe_value() const
    {
        return *reinterpret_cast<const Type*>(&_memory.memory);
    }

    union Memory {
        Memory() { }
        ~Memory() { }
        Type memory;
    } _memory;

    bool _none;
};

#else

/// Emulate std::optional. May or may not contain an object of type Type. Use operator bool to test.
template <typename Type>
class Optional {
public:
    // TODO: emplace, and emplace constructors

    Optional()
        : _none(true)
    {
    }

    Optional(NullOptional)
        : _none(true)
    {
    }

    ~Optional()
    {
        clear();
    }

    Optional& operator=(NullOptional)
    {
        clear();
        return *this;
    }

    Optional(const Type& copy)
        : _none(false)
    {
        new (_memory.raw) Type(copy);
    }

    Optional& operator=(const Type& copy)
    {
        if (_none) {
            _none = false;
            new (_memory.raw) Type(copy);
        } else {
            unsafe_value() = copy;
        }
        return *this;
    }

#ifdef PRIME_COMPILER_RVALUEREF

    Optional(Type&& rvalue) PRIME_NOEXCEPT : _none(false)
    {
        new (_memory.raw) Type(std::move(rvalue));
    }

    Optional& operator=(Type&& rvalue) PRIME_NOEXCEPT
    {
        if (_none) {
            _none = false;
            new (_memory.raw) Type(std::move(rvalue));
        } else {
            unsafe_value() = std::move(rvalue);
        }
        return *this;
    }

#endif

    Optional(const Optional& copy)
        : _none(copy._none)
    {
        if (!_none) {
            new (_memory.raw) Type(copy.unsafe_value());
        }
    }

    Optional& operator=(const Optional& copy)
    {
        if (copy._none) {
            clear();
        } else {
            operator=(copy.value());
        }
        return *this;
    }

#ifdef PRIME_COMPILER_RVALUEREF

    Optional(Optional&& rvalue) PRIME_NOEXCEPT : _none(rvalue._none)
    {
        if (!_none) {
            new (_memory.raw) Type(std::move(rvalue.unsafe_value()));
            rvalue.clear();
        }
    }

    Optional& operator=(Optional&& rvalue) PRIME_NOEXCEPT
    {
        if (rvalue._none) {
            clear();
        } else {
            operator=(std::move(rvalue.unsafe_value()));
            rvalue.clear();
        }
        return *this;
    }

#endif

    Type& value()
    {
        PRIME_DEBUG_ASSERT(!_none);
        return unsafe_value();
    }

    const Type& value() const
    {
        PRIME_DEBUG_ASSERT(!_none);
        return unsafe_value();
    }

    Type value_or(const Type& or_value) const
    {
        return _none ? or_value : unsafe_value();
    }

#ifdef PRIME_COMPILER_RVALUEREF
    Type value_or(Type&& or_value) const
    {
        return _none ? std::move(or_value) : unsafe_value();
    }
#endif

    Type* operator->()
    {
        return &value();
    }

    const Type* operator->() const
    {
        return &value();
    }

#ifdef PRIME_COMPILER_RVALUEREF

    Type& operator*() &
    {
        return value();
    }

    const Type& operator*() const&
    {
        return value();
    }

    const Type&& operator*() const&&
    {
        return std::move(value());
    }

    Type&& operator*() &&
    {
        return std::move(value());
    }

#else

    Type& operator*()
    {
        return value();
    }

    const Type& operator*() const
    {
        return value();
    }

#endif

    bool operator!() const
    {
        return _none;
    }

#ifdef PRIME_COMPILER_EXPLICIT_CONVERSION
    explicit operator bool() const
    {
        return !_none;
    }
#endif

    bool has_value() const
    {
        return !_none;
    }

    bool operator==(const Optional& rhs) const
    {
        if (_none) {
            return rhs._none;
        } else if (rhs._none) {
            return false;
        }
        return unsafe_value() == rhs.unsafe_value();
    }

    bool operator<(const Optional& rhs) const
    {
        if (_none) {
            return !rhs._none;
        } else if (rhs._none) {
            return false;
        }
        return unsafe_value() < rhs.unsafe_value();
    }

    PRIME_IMPLIED_COMPARISONS_OPERATORS(const Optional&)

    bool operator==(NullOptional) const
    {
        return _none;
    }

    bool operator!=(NullOptional) const
    {
        return !_none;
    }

    void swap(Optional& with)
    {
        if (_none) {
            if (with._none) {
                return;
            }

            new (_memory.raw) Type(PRIME_MOVE(with.unsafe_value()));
            _none = false;
            with.clear();
            return;
        }

        if (with._none) {
            new (with._memory.raw) Type(PRIME_MOVE(unsafe_value()));
            with._none = false;
            clear();
            return;
        }

        Type temp(PRIME_MOVE(unsafe_value()));
        unsafe_value() = PRIME_MOVE(with.unsafe_value());
        with.unsafe_value() = PRIME_MOVE(temp);
    }

private:
    void clear()
    {
        if (!_none) {
            unsafe_value().~Type();
            _none = true;
        }
    }

    Optional(Null)
        : _none(true)
    {
    }

    Optional& operator=(Null)
    {
        clear();
        return *this;
    }

    bool operator==(Null) const
    {
        return _none;
    }

    bool operator!=(Null) const
    {
        return !_none;
    }

    Type& unsafe_value()
    {
        return *reinterpret_cast<Type*>(_memory.raw);
    }

    const Type& unsafe_value() const
    {
        return *reinterpret_cast<const Type*>(_memory.raw);
    }

    union {
        PRIME_ALIGNED_TYPE(PRIME_ALIGNOF(Type))
        align;
        char raw[sizeof(Type)];
    } _memory;

    bool _none;
};

#endif

/// Hide operator ! to avoid a dangerous ambiguity
class OptionalBool : public Optional<bool> {
public:
    typedef Optional<bool> Super;

    OptionalBool()
    {
    }

    OptionalBool(NullOptional value)
        : Super(value)
    {
    }

    OptionalBool& operator=(NullOptional value)
    {
        Super::operator=(value);
        return *this;
    }

    OptionalBool(bool value)
        : Super(value)
    {
    }

    OptionalBool& operator=(bool value)
    {
        Super::operator=(value);
        return *this;
    }

private:
    bool operator!() const { return !Super::has_value(); }

#ifdef PRIME_COMPILER_EXPLICIT_CONVERSION
    explicit operator bool() const
    {
        return Super::has_value();
    }
#endif
};
}

#endif
