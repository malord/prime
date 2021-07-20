// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STRINGVIEW_H
#define PRIME_STRINGVIEW_H

#include "StringSearching.h"
#include <algorithm>
#include <string.h>
#include <string>
#include <vector>
#include <wchar.h>
#ifdef PRIME_ENABLE_IOSTREAMS
#include <ostream>
#endif

namespace Prime {

/// Under C++ 17, this will be replaced to just use std::string_view

/// A StringView is a reference to a an immutable array of characters in memory. It does not copy the memory it is
/// initialised with so it's up to the user to ensure that the memory remains valid for the lifetime of the
/// StringView. StringView can be safely used for function arguments, allowing string literals and STL strings
/// to be passed without incurring copying. Use for return values is not recommended as that has all the safety
/// of returning a reference with non of the compiler warnings to help you get it right.
/// Occasionally updated to keep compatibility with the C++ standards committee's experimental string_view.
template <typename Char, typename Traits = std::char_traits<Char>>
class BasicStringView : public StringSearching<Char, BasicStringView<Char, Traits>, size_t, Traits> {
public:
#ifdef PRIME_CXX11
    struct Less {
        using is_transparent = std::true_type;

        template <typename Other>
        bool operator()(const BasicStringView& lhs, const Other& other) const
        {
            return lhs < other;
        }
    };
#endif

    typedef Char value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef const Char* const_pointer;
    typedef const Char& const_reference;
    typedef const Char* iterator;
    typedef const Char* const_iterator;
#ifndef PRIME_COMPILER_NO_REVERSE_ITERATOR
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
#endif

    typedef Traits traits_type;

    PRIME_STATIC_CONST(size_type, npos, static_cast<size_type>(-1));

    typedef StringSearching<Char, BasicStringView<Char, Traits>, size_t, Traits> searching_type;

    //
    // Construction
    //

    BasicStringView() PRIME_NOEXCEPT : _string(emptyString),
                                       _size(0)
    {
    }

    BasicStringView(const BasicStringView& copy) PRIME_NOEXCEPT : _string(copy._string),
                                                                  _size(copy._size)
    {
    }

    BasicStringView(const Char* string) PRIME_NOEXCEPT
    {
        assign(string);
    }

    BasicStringView(const Char* array, size_type arraySize) PRIME_NOEXCEPT : _string(array),
                                                                             _size(arraySize)
    {
    }

    BasicStringView(const Char* begin, const Char* end) PRIME_NOEXCEPT : _string(begin),
                                                                         _size((size_type)(end - begin))
    {
    }

    BasicStringView(BasicStringView other, size_type otherOffset, size_type otherSize = npos) PRIME_NOEXCEPT
    {
        assign(other, otherOffset, otherSize);
    }

    template <typename StringTraits, typename StringAllocator>
    BasicStringView(const std::basic_string<Char, StringTraits, StringAllocator>& container) PRIME_NOEXCEPT : _string(container.data()),
                                                                                                              _size(container.size())
    {
    }

    //
    // Assignment
    //

    void assign(BasicStringView copy) PRIME_NOEXCEPT
    {
        _string = copy._string;
        _size = copy._size;
    }

    void assign(const Char* array, size_type arraySize) PRIME_NOEXCEPT
    {
        _string = array;
        _size = arraySize;
    }

    void assign(const Char* begin, const Char* end) PRIME_NOEXCEPT
    {
        _string = begin;
        _size = (size_type)(end - begin);
    }

    void assign(BasicStringView other, size_type otherOffset, size_type otherSize = npos) PRIME_NOEXCEPT
    {
        searching_type::fix_offset_count(other, otherOffset, otherSize);
        _string = other.data() + otherOffset;
        _size = otherSize;
    }

    void assign(const Char* string) PRIME_NOEXCEPT
    {
        //if (PRIME_DEBUG_GUARD(string)) {
        if (string) {
            _string = string;
            _size = traits_type::length(string);
        } else {
            _string = emptyString;
            _size = 0;
        }
    }

    BasicStringView& operator=(const BasicStringView& copy) PRIME_NOEXCEPT
    {
        _string = copy._string;
        _size = copy._size;
        return *this;
    }

#ifdef PRIME_COMPILER_RVALUEREF
    BasicStringView& operator=(BasicStringView&& other) PRIME_NOEXCEPT
    {
        if (this != &other) {
            _string = other._string;
            _size = other._size;
        }

        return *this;
    }
#endif

    template <typename StringTraits, typename StringAllocator>
    BasicStringView& operator=(const std::basic_string<Char, StringTraits, StringAllocator>& container) PRIME_NOEXCEPT
    {
        _string = container.data();
        _size = container.size();
        return *this;
    }

    BasicStringView& operator=(const Char* string) PRIME_NOEXCEPT
    {
        assign(string);
        return *this;
    }

    //
    // Access
    //

    bool empty() const PRIME_NOEXCEPT { return size() == 0; }

    const Char* data() const PRIME_NOEXCEPT { return _string; }

    const_iterator begin() const PRIME_NOEXCEPT { return _string; }
    const_iterator end() const PRIME_NOEXCEPT { return _string + size(); }

    const_iterator cbegin() const PRIME_NOEXCEPT { return _string; }
    const_iterator cend() const PRIME_NOEXCEPT { return _string + size(); }

#ifndef PRIME_COMPILER_NO_REVERSE_ITERATOR
    const_reverse_iterator rbegin() const PRIME_NOEXCEPT
    {
        return const_reverse_iterator(cend());
    }
    const_reverse_iterator rend() const PRIME_NOEXCEPT { return const_reverse_iterator(cbegin()); }

    const_reverse_iterator crbegin() const PRIME_NOEXCEPT { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const PRIME_NOEXCEPT { return const_reverse_iterator(cbegin()); }
#endif

    const Char& front() const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!empty());
        return _string[0];
    }
    const Char& back() const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!empty());
        return _string[size() - 1];
    }

    size_type size() const PRIME_NOEXCEPT { return _size; }
    size_type length() const PRIME_NOEXCEPT { return size(); }
    size_type max_size() const PRIME_NOEXCEPT { return ((size_type)-1) / sizeof(Char); }

    template <typename Index>
    const Char& operator[](Index index) const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT((size_type)index < size());
        return _string[(size_type)index];
    }

    template <typename Index>
    Char at(Index index) const PRIME_NOEXCEPT
    {
        return static_cast<size_type>(index) < size() ? _string[index] : Char(0);
    }

    //
    // Substrings
    //

    BasicStringView substr(size_type offset = 0, size_type count = npos) const PRIME_NOEXCEPT
    {
        return BasicStringView(*this, offset, count);
    }

    void copy(Char* dest, size_type n, size_type pos = 0) const PRIME_NOEXCEPT
    {
        searching_type::fix_offset_count(*this, pos, n);
        std::copy(_string + pos, _string + pos + n, dest);
    }

    //
    // Conversion
    //

#if PRIME_MSC_AND_OLDER(1300)
    std::basic_string<Char, Traits, std::allocator<Char>> to_string(const std::allocator<Char>& a = std::allocator<Char>()) const
    {
        return std::basic_string<Char, Traits>(data(), size(), a);
    }
#else
    template <typename StringAllocator = std::allocator<Char>>
    std::basic_string<Char, Traits, StringAllocator> to_string(const StringAllocator& a = StringAllocator()) const
    {
        return std::basic_string<Char, Traits>(data(), size(), a);
    }
#endif

#ifdef PRIME_COMPILER_EXPLICIT_CONVERSION

    template <typename StringAllocator = std::allocator<Char>>
    explicit operator std::basic_string<Char, Traits, StringAllocator>()
    {
        return std::basic_string<Char, Traits, StringAllocator>(data(), size());
    }

#endif

    //
    // Modifiers
    //

    void remove_suffix(size_type n = 1) PRIME_NOEXCEPT
    {
        if (n && PRIME_DEBUG_GUARD(size() >= n)) {
            _size -= n;
        }
    }

    void remove_prefix(size_type n = 1) PRIME_NOEXCEPT
    {
        if (n && PRIME_DEBUG_GUARD(size() >= n)) {
            _size -= n;
            _string += n;
        }
    }

    void swap(BasicStringView& other) PRIME_NOEXCEPT
    {
        const_pointer tempString = _string;
        size_type tempSize = _size;

        _string = other._string;
        _size = other._size;

        other._string = tempString;
        other._size = tempSize;
    }

    //
    // Comparisons
    //

    bool operator==(BasicStringView other) const PRIME_NOEXCEPT
    {
        return size() == other.size() && (!size() || std::equal(_string, _string + size(), other._string));
    }

    bool operator<(BasicStringView other) const PRIME_NOEXCEPT
    {
        if (empty()) {
            return !other.empty();
        }

        if (other.empty()) {
            return false;
        }

        return std::lexicographical_compare(_string, _string + size(), other._string, other._string + other.size());
    }

    PRIME_IMPLIED_COMPARISONS_OPERATORS(BasicStringView)

    bool equal(BasicStringView other) const PRIME_NOEXCEPT
    {
        return size() == other.size() && !empty() && memcmp(data(), other.data(), size()) == 0;
    }

private:
    const_pointer _string;
    size_type _size;

    static const Char emptyString[1];
};

template <typename Char, typename Traits>
const Char BasicStringView<Char, Traits>::emptyString[1] = { Char(0) };

//
// Comparison operators
//

template <typename Char, typename Traits>
inline bool operator==(const BasicStringView<Char, Traits>& lhs, const Char* rhs) PRIME_NOEXCEPT { return lhs.operator==(BasicStringView<Char, Traits>(rhs)); }
template <typename Char, typename Traits>
inline bool operator!=(const BasicStringView<Char, Traits>& lhs, const Char* rhs) PRIME_NOEXCEPT { return lhs.operator!=(BasicStringView<Char, Traits>(rhs)); }
template <typename Char, typename Traits>
inline bool operator<(const BasicStringView<Char, Traits>& lhs, const Char* rhs) PRIME_NOEXCEPT { return lhs.operator<(BasicStringView<Char, Traits>(rhs)); }
template <typename Char, typename Traits>
inline bool operator>(const BasicStringView<Char, Traits>& lhs, const Char* rhs) PRIME_NOEXCEPT { return lhs.operator>(BasicStringView<Char, Traits>(rhs)); }
template <typename Char, typename Traits>
inline bool operator<=(const BasicStringView<Char, Traits>& lhs, const Char* rhs) PRIME_NOEXCEPT { return lhs.operator<=(BasicStringView<Char, Traits>(rhs)); }
template <typename Char, typename Traits>
inline bool operator>=(const BasicStringView<Char, Traits>& lhs, const Char* rhs) PRIME_NOEXCEPT { return lhs.operator>=(BasicStringView<Char, Traits>(rhs)); }

template <typename Char, typename Traits>
inline bool operator==(const Char* lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator==(rhs); }
template <typename Char, typename Traits>
inline bool operator!=(const Char* lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator!=(rhs); }
template <typename Char, typename Traits>
inline bool operator<(const Char* lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator<(rhs); }
template <typename Char, typename Traits>
inline bool operator>(const Char* lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator>(rhs); }
template <typename Char, typename Traits>
inline bool operator<=(const Char* lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator<=(rhs); }
template <typename Char, typename Traits>
inline bool operator>=(const Char* lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator>=(rhs); }

template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator==(const BasicStringView<Char, Traits>& lhs, const std::basic_string<Char, StringTraits, StringAllocator>& rhs) PRIME_NOEXCEPT { return lhs.operator==(BasicStringView<Char, Traits>(rhs)); }
template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator!=(const BasicStringView<Char, Traits>& lhs, const std::basic_string<Char, StringTraits, StringAllocator>& rhs) PRIME_NOEXCEPT { return lhs.operator!=(BasicStringView<Char, Traits>(rhs)); }
template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator<(const BasicStringView<Char, Traits>& lhs, const std::basic_string<Char, StringTraits, StringAllocator>& rhs) PRIME_NOEXCEPT { return lhs.operator<(BasicStringView<Char, Traits>(rhs)); }
template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator>(const BasicStringView<Char, Traits>& lhs, const std::basic_string<Char, StringTraits, StringAllocator>& rhs) PRIME_NOEXCEPT { return lhs.operator>(BasicStringView<Char, Traits>(rhs)); }
template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator<=(const BasicStringView<Char, Traits>& lhs, const std::basic_string<Char, StringTraits, StringAllocator>& rhs) PRIME_NOEXCEPT { return lhs.operator<=(BasicStringView<Char, Traits>(rhs)); }
template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator>=(const BasicStringView<Char, Traits>& lhs, const std::basic_string<Char, StringTraits, StringAllocator>& rhs) PRIME_NOEXCEPT { return lhs.operator>=(BasicStringView<Char, Traits>(rhs)); }

template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator==(const std::basic_string<Char, StringTraits, StringAllocator>& lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator==(rhs); }
template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator!=(const std::basic_string<Char, StringTraits, StringAllocator>& lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator!=(rhs); }
template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator<(const std::basic_string<Char, StringTraits, StringAllocator>& lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator<(rhs); }
template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator>(const std::basic_string<Char, StringTraits, StringAllocator>& lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator>(rhs); }
template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator<=(const std::basic_string<Char, StringTraits, StringAllocator>& lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator<=(rhs); }
template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline bool operator>=(const std::basic_string<Char, StringTraits, StringAllocator>& lhs, const BasicStringView<Char, Traits>& rhs) PRIME_NOEXCEPT { return BasicStringView<Char, Traits>(lhs).operator>=(rhs); }

//
// Concatenation operators
//

template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline std::basic_string<Char, Traits>& operator+=(std::basic_string<Char, StringTraits, StringAllocator>& lhs, BasicStringView<Char, Traits> rhs)
{
    return lhs.append(rhs.data(), rhs.size());
}

template <typename Char, typename Traits>
inline std::basic_string<Char, Traits> operator+(const BasicStringView<Char, Traits>& lhs, const BasicStringView<Char, Traits>& rhs)
{
    std::basic_string<Char, Traits> result;
    result.reserve(lhs.size() + rhs.size());
    result.append(lhs.begin(), lhs.end());
    result.append(rhs.begin(), rhs.end());
    return result;
}

template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline std::basic_string<Char, Traits> operator+(const BasicStringView<Char, Traits>& lhs, const std::basic_string<Char, StringTraits, StringAllocator>& rhs)
{
    std::basic_string<Char, Traits> result;
    result.reserve(lhs.size() + rhs.size());
    result.append(lhs.begin(), lhs.end());
    result.append(rhs);
    return result;
}

template <typename Char, typename Traits, typename StringTraits, typename StringAllocator>
inline std::basic_string<Char, Traits> operator+(const std::basic_string<Char, StringTraits, StringAllocator>& lhs, const BasicStringView<Char, Traits>& rhs)
{
    std::basic_string<Char, Traits> result;
    result.reserve(lhs.size() + rhs.size());
    result.append(lhs);
    result.append(rhs.begin(), rhs.end());
    return result;
}

template <typename Char, typename Traits>
inline std::basic_string<Char, Traits> operator+(const BasicStringView<Char, Traits>& lhs, const Char* rhs)
{
    std::basic_string<Char, Traits> result;
    result.append(lhs.begin(), lhs.end());
    result.append(rhs);
    return result;
}

template <typename Char, typename Traits>
inline std::basic_string<Char, Traits> operator+(const Char* lhs, const BasicStringView<Char, Traits>& rhs)
{
    std::basic_string<Char, Traits> result;
    result.append(lhs);
    result.append(rhs.begin(), rhs.end());
    return result;
}

template <typename Char, typename Traits>
inline std::basic_string<Char, Traits> operator+(const BasicStringView<Char, Traits>& lhs, Char rhs)
{
    std::basic_string<Char, Traits> result;
    result.reserve(lhs.size() + 1);
    result.append(lhs.begin(), lhs.end());
    result += rhs;
    return result;
}

template <typename Char, typename Traits>
inline std::basic_string<Char, Traits> operator+(Char lhs, const BasicStringView<Char, Traits>& rhs)
{
    std::basic_string<Char, Traits> result;
    result.reserve(rhs.size() + 1);
    result += lhs;
    result.append(rhs.begin(), rhs.end());
    return result;
}

//
// Typedefs
//

typedef BasicStringView<char> StringView;

typedef BasicStringView<wchar_t> WideStringView;

//
// MaybeNullTerminatedStringView
//

/// Utility class to help out code which need a null terminated string for legacy APIs.
template <typename Char, typename Traits = std::char_traits<Char>>
class BasicMaybeNullTerminatedStringView {
public:
    typedef Char value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef const Char* const_pointer;
    typedef const Char& const_reference;
    typedef const Char* iterator;
    typedef const Char* const_iterator;
#ifndef PRIME_COMPILER_NO_REVERSE_ITERATOR
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
#endif
    typedef Traits traits_type;

    template <typename StringTraits, typename StringAllocator>
    BasicMaybeNullTerminatedStringView(const std::basic_string<Char, StringTraits, StringAllocator>& container) PRIME_NOEXCEPT : _begin(container.c_str()),
                                                                                                                                 _end(NULL)
    {
    }

    BasicMaybeNullTerminatedStringView(const Char* stringz) PRIME_NOEXCEPT : _begin(stringz),
                                                                             _end(NULL)
    {
    }

    BasicMaybeNullTerminatedStringView(const BasicStringView<Char, Traits>& view) PRIME_NOEXCEPT : _begin(view.begin()),
                                                                                                   _end(view.end())
    {
    }

    template <typename StringTraits>
    BasicMaybeNullTerminatedStringView(const BasicStringView<Char, StringTraits>& view) PRIME_NOEXCEPT : _begin(view.begin()),
                                                                                                         _end(view.end())
    {
    }

    BasicMaybeNullTerminatedStringView(const BasicMaybeNullTerminatedStringView& rhs) PRIME_NOEXCEPT : _begin(rhs._begin),
                                                                                                       _end(rhs._end)
    {
    }

    BasicMaybeNullTerminatedStringView& operator=(BasicMaybeNullTerminatedStringView& rhs) PRIME_NOEXCEPT
    {
        _begin = rhs._begin;
        _end = rhs._end;
        return *this;
    }

#ifdef PRIME_COMPILER_RVALUEREF
    BasicMaybeNullTerminatedStringView(BasicMaybeNullTerminatedStringView&& rhs) = default;

    BasicMaybeNullTerminatedStringView& operator=(BasicMaybeNullTerminatedStringView&& rhs) = default;
#endif

    bool null_terminated() const PRIME_NOEXCEPT
    {
        return _end == NULL;
    }

    const Char* c_str() const PRIME_NOEXCEPT
    {
        PRIME_ASSERT(null_terminated());
        return _begin;
    }

    const std::basic_string<Char> to_string() const
    {
        return std::basic_string<Char>(begin(), end());
    }

    const Char* data() const PRIME_NOEXCEPT { return _begin; }

    size_type size() const PRIME_NOEXCEPT { return _end ? size_type(_end - _begin) : traits_type::length(_begin); }
    size_type length() const PRIME_NOEXCEPT { return size(); }

    bool empty() PRIME_NOEXCEPT { return _end == NULL ? (!*_begin) : _end == _begin; }

    const Char* begin() const PRIME_NOEXCEPT { return _begin; }
    const Char* end() const PRIME_NOEXCEPT { return _end ? _end : find_end(); }

    const Char* cbegin() const PRIME_NOEXCEPT { return begin(); }
    const Char* cend() const PRIME_NOEXCEPT { return end(); }

    const Char& front() const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!empty());
        return _begin[0];
    }
    const Char& back() const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!empty());
        return _begin[size() - 1];
    }

#ifndef PRIME_COMPILER_NO_REVERSE_ITERATOR
    const_reverse_iterator rbegin() const PRIME_NOEXCEPT
    {
        return const_reverse_iterator(cend());
    }
    const_reverse_iterator rend() const PRIME_NOEXCEPT { return const_reverse_iterator(cbegin()); }

    const_reverse_iterator crbegin() const PRIME_NOEXCEPT { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const PRIME_NOEXCEPT { return const_reverse_iterator(cbegin()); }
#endif

    template <typename Index>
    const Char& operator[](Index index) const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT((size_type)index < size());
        return _begin[(size_type)index];
    }

    template <typename Index>
    Char at(Index index) const PRIME_NOEXCEPT
    {
        return index < size() ? _begin[index] : Char(0);
    }

    BasicStringView<Char> to_view() const PRIME_NOEXCEPT { return BasicStringView<Char>(begin(), end()); }

    void remove_prefix(size_type n = 1) PRIME_NOEXCEPT
    {
        if (n && PRIME_DEBUG_GUARD(size() >= n)) {
            _begin += n;
        }
    }

private:
    const Char* find_end() const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(_end == NULL);
        return _begin + traits_type::length(_begin);
    }

    const Char* _begin;
    const Char* _end;
};

typedef BasicMaybeNullTerminatedStringView<char> MaybeNullTerminatedStringView;

//
// ostream
//

#ifdef PRIME_ENABLE_IOSTREAMS
template <typename Char, typename OStreamTraits>
std::basic_ostream<Char, OStreamTraits>& operator<<(std::basic_ostream<Char, OStreamTraits>& os,
    const BasicStringView<Char, Traits>& string)
{
    os.write(string.data(), string.size());
    return os;
}

template <typename Char, typename OStreamTraits>
std::basic_ostream<Char, OStreamTraits>& operator<<(std::basic_ostream<Char, OStreamTraits>& os,
    const BasicMaybeNullTerminatedStringView<Char, Traits>& string)
{
    os.write(string.data(), string.size());
    return os;
}
#endif

}

#endif
