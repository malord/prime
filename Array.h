// Copyright 2000-2021 Mark H. P. Lord

// std::array for C++03

#ifndef PRIME_ARRAY_H
#define PRIME_ARRAY_H

#include "Config.h"

#if defined(PRIME_CXX11_STL) && 0

//
// Use C++11 std::array if we have it
//

#include <array>

namespace Prime {

template <typename Type, size_t ArraySize>
using Array = std::array<Type, ArraySize>;
}

#else

//
// Emulate C++11 std::array
//

#include <algorithm>
#include <iterator>

namespace Prime {

/// C++11 std::array-alike for platforms which don't have it.
template <typename Type, size_t ArraySize>
class Array {
public:
    typedef Type value_type;

    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    typedef Type* pointer;
    typedef const Type* const_pointer;

    typedef Type& reference;
    typedef const Type& const_reference;

    typedef Type* iterator;
    typedef const Type* const_iterator;

#ifndef PRIME_COMPILER_NO_REVERSE_ITERATOR
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
#endif

    Array() PRIME_NOEXCEPT
    {
    }

    explicit Array(const Type* input) PRIME_NOEXCEPT
    {
        std::copy(input, input + ArraySize, _array);
    }

    Array(const Array& other) PRIME_NOEXCEPT
    {
        std::copy(other._array, other._array + ArraySize, _array);
    }

    Array& operator=(const Array& other) PRIME_NOEXCEPT
    {
        if (this != &other) {
            std::copy(other._array, other._array + ArraySize, _array);
        }

        return *this;
    }

#ifdef PRIME_COMPILER_RVALUEREF
    Array(Array&& other) PRIME_NOEXCEPT
    {
        std::copy(std::make_move_iterator(&other._array[0]), std::make_move_iterator(&other._array[0] + ArraySize), _array);
    }

    Array& operator=(Array&& other) PRIME_NOEXCEPT
    {
        if (&other != this) {
            std::copy(std::make_move_iterator(&other._array[0]), std::make_move_iterator(&other._array[0] + ArraySize), _array);
        }

        return *this;
    }
#endif

    bool empty() const PRIME_NOEXCEPT
    {
        return ArraySize == 0;
    }

    size_type size() const PRIME_NOEXCEPT { return ArraySize; }
    size_type max_size() const PRIME_NOEXCEPT { return (size_type)-1 / sizeof(Type); }

    pointer begin() PRIME_NOEXCEPT { return _array; }
    pointer end() PRIME_NOEXCEPT { return _array + ArraySize; }
    const_pointer begin() const PRIME_NOEXCEPT { return _array; }
    const_pointer end() const PRIME_NOEXCEPT { return _array + ArraySize; }

    const_pointer cbegin() const PRIME_NOEXCEPT { return _array; }
    const_pointer cend() const PRIME_NOEXCEPT { return _array + ArraySize; }

    pointer data() PRIME_NOEXCEPT { return _array; }
    const_pointer data() const PRIME_NOEXCEPT { return _array; }

#ifndef PRIME_COMPILER_NO_REVERSE_ITERATOR
    reverse_iterator rbegin() PRIME_NOEXCEPT
    {
        return reverse_iterator(end());
    }
    reverse_iterator rend() PRIME_NOEXCEPT { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const PRIME_NOEXCEPT { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const PRIME_NOEXCEPT { return const_reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const PRIME_NOEXCEPT { return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const PRIME_NOEXCEPT { return const_reverse_iterator(begin()); }
#endif

    operator Type*() PRIME_NOEXCEPT
    {
        return _array;
    }
    operator const Type*() const PRIME_NOEXCEPT { return _array; }

    template <typename Index>
    const Type& operator[](Index which) const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT((size_type)which < size());
        return _array[which];
    }

    template <typename Index>
    Type& operator[](Index which) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT((size_type)which < size());
        return _array[which];
    }

    template <typename Index>
    const Type& at(Index which) const PRIME_NOEXCEPT
    {
        PRIME_ASSERT((size_type)which < size());
        return _array[which];
    }

    template <typename Index>
    Type& at(Index which) PRIME_NOEXCEPT
    {
        PRIME_ASSERT((size_type)which < size());
        return _array[which];
    }

    const Type& back() const PRIME_NOEXCEPT { return _array[ArraySize - 1]; }
    Type& back() PRIME_NOEXCEPT { return _array[ArraySize - 1]; }

    const Type& front() const PRIME_NOEXCEPT { return _array[0]; }
    Type& front() PRIME_NOEXCEPT { return _array[0]; }

    bool operator==(const Array& other) const PRIME_NOEXCEPT
    {
        return std::equal(_array, _array + ArraySize, other._array);
    }

    bool operator<(const Array& other) const PRIME_NOEXCEPT
    {
        return std::lexicographical_compare(_array, _array + ArraySize, other._array, other._array + ArraySize);
    }

    PRIME_IMPLIED_COMPARISONS_OPERATORS(const Array&)

    void fill(const Type& value) PRIME_NOEXCEPT
    {
        std::fill(begin(), end(), value);
    }

    void swap(Array& a, Array& b) PRIME_NOEXCEPT
    {
        std::swap_ranges(a.begin(), a.end(), b.begin());
    }

private:
    operator void*() { return NULL; }

    operator const void*() const { return NULL; }

    Type _array[ArraySize];
};

template <typename Type, size_t ArraySize>
inline void swap(Array<Type, ArraySize>& a, Array<Type, ArraySize>& b) PRIME_NOEXCEPT
{
    a.swap(b);
}
}

#endif // C++11

#endif
