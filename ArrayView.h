// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ARRAYVIEW_H
#define PRIME_ARRAYVIEW_H

#include "Config.h"
#include <algorithm>
#include <iterator>
#ifdef PRIME_ENABLE_IOSTREAMS
#include <ostream>
#endif

namespace Prime {

/// An ArrayView is a reference to a mutable or immutable array. It does not copy the elements it is initialised
/// with so it's up to the user to ensure that the memory remains valid for the lifetime of the ArrayView.
/// ArrayView can be safely used for function arguments, allowing any contiguous container to be passed without
/// incurring copying. Use for return values is not recommended as that has all the safety of returning a
/// reference with none of the compiler warnings.
/// Designed to be compatible with the future C++ std::array_view, but currently only supports one-dimensional
/// arrays.
template <typename Element>
class ArrayView {
public:
    typedef Element value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef Element* pointer;
    typedef const Element* const_pointer;
    typedef Element& reference;
    typedef const Element& const_reference;
    typedef Element* iterator;
    typedef const Element* const_iterator;
#ifndef PRIME_COMPILER_NO_REVERSE_ITERATOR
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
#endif

    //
    // Construction
    //

    ArrayView() PRIME_NOEXCEPT : _array(NULL),
                                 _length(0)
    {
    }

    ArrayView(const ArrayView& copy) PRIME_NOEXCEPT : _array(copy._array),
                                                      _length(copy._length)
    {
    }

    ArrayView(pointer array, size_type length) PRIME_NOEXCEPT : _array(array),
                                                                _length(length)
    {
    }

    ArrayView(pointer begin, pointer end) PRIME_NOEXCEPT : _array(begin),
                                                           _length((size_type)(end - begin))
    {
    }

#ifndef PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES
    template <size_type autoArraySize>
    ArrayView(Element (&array)[autoArraySize]) PRIME_NOEXCEPT : _array(&array[0]),
                                                                _length(autoArraySize)
    {
    }
#endif

#if !PRIME_MSC_AND_OLDER(1300)
    /// The second parameter is just to ensure we have a container.
    template <typename Container>
    ArrayView(Container& container, typename Container::const_iterator* = NULL) PRIME_NOEXCEPT : _array(container.data()),
                                                                                                 _length(container.size())
    {
    }
#endif

#ifdef PRIME_COMPILER_INITLIST
    ArrayView(std::initializer_list<Element> list) PRIME_NOEXCEPT : _array(list.begin()),
                                                                    _length(list.size())
    {
    }
#endif

    //
    // Assignment
    //

    ArrayView& operator=(const ArrayView& copy) PRIME_NOEXCEPT
    {
        _array = copy._array;
        _length = copy._length;
        return *this;
    }

#ifdef PRIME_COMPILER_RVALUEREF
    ArrayView& operator=(ArrayView&& other) PRIME_NOEXCEPT
    {
        if (this != &other) {
            _array = other._array;
            _length = other._length;
        }

        return *this;
    }
#endif

    //
    // Access
    //

    bool empty() const PRIME_NOEXCEPT { return size() == 0; }

    const_pointer data() const PRIME_NOEXCEPT { return _array; }
    pointer data() PRIME_NOEXCEPT { return _array; }

    iterator begin() PRIME_NOEXCEPT { return _array; }
    iterator end() PRIME_NOEXCEPT { return _array + size(); }

    const_iterator begin() const PRIME_NOEXCEPT { return _array; }
    const_iterator end() const PRIME_NOEXCEPT { return _array + size(); }

    const_iterator cbegin() const PRIME_NOEXCEPT { return _array; }
    const_iterator cend() const PRIME_NOEXCEPT { return _array + size(); }

#ifndef PRIME_COMPILER_NO_REVERSE_ITERATOR
    reverse_iterator rbegin() PRIME_NOEXCEPT
    {
        return reverse_iterator(end());
    }
    reverse_iterator rend() PRIME_NOEXCEPT { return reverse_iterator(begin()); }

    const_reverse_iterator rbegin() const PRIME_NOEXCEPT { return const_reverse_iterator(cend()); }
    const_reverse_iterator rend() const PRIME_NOEXCEPT { return const_reverse_iterator(cbegin()); }

    const_reverse_iterator crbegin() const PRIME_NOEXCEPT { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const PRIME_NOEXCEPT { return const_reverse_iterator(cbegin()); }
#endif

    Element& front() PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!empty());
        return _array[0];
    }
    const Element& front() const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!empty());
        return _array[0];
    }
    Element& back() PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!empty());
        return _array[size() - 1];
    }
    const Element& back() const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!empty());
        return _array[size() - 1];
    }

    size_type size() const PRIME_NOEXCEPT { return _length; }
    size_type length() const PRIME_NOEXCEPT { return size(); }
    size_type max_size() const PRIME_NOEXCEPT { return ((size_type)-1) / sizeof(Element); }
    size_type bytes() const PRIME_NOEXCEPT { return _length * sizeof(Element); }

    size_type used_length() const PRIME_NOEXCEPT { return length(); }
    size_type used_bytes() const PRIME_NOEXCEPT { return bytes(); }

    template <typename Index>
    Element& operator[](Index index) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT((size_type)index < size());
        return _array[(size_type)index];
    }

    template <typename Index>
    const Element& operator[](Index index) const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT((size_type)index < size());
        return _array[(size_type)index];
    }

    template <typename Index>
    Element& at(Index index) PRIME_NOEXCEPT
    {
        PRIME_ASSERT((size_type)index < size());
        return _array[(size_type)index];
    }

    template <typename Index>
    const Element& at(Index index) const PRIME_NOEXCEPT
    {
        PRIME_ASSERT((size_type)index < size());
        return _array[(size_type)index];
    }

    //
    // Comparisons
    //

    bool operator==(ArrayView other) const PRIME_NOEXCEPT
    {
        return size() == other.size() && (!size() || std::equal(_array, _array + size(), other._array));
    }

    bool operator<(ArrayView other) const PRIME_NOEXCEPT
    {
        if (empty()) {
            return !other.empty();
        }

        if (other.empty()) {
            return false;
        }

        return std::lexicographical_compare(_array, _array + size(), other._array, other._array + other.size());
    }

    PRIME_IMPLIED_COMPARISONS_OPERATORS(ArrayView)

    bool equal(ArrayView other) const PRIME_NOEXCEPT
    {
        return size() == other.size() && !empty() && memcmp(data(), other.data(), size()) == 0;
    }

    //
    // Slicing
    //

    // TODO: std::array_view has/had first<count>

    ArrayView first(size_type count) PRIME_NOEXCEPT
    {
        if (!PRIME_DEBUG_GUARD(count <= size())) {
            count = size();
        }
        return ArrayView(data(), count);
    }

    ArrayView last(size_type count) PRIME_NOEXCEPT
    {
        if (!PRIME_DEBUG_GUARD(count <= size())) {
            count = size();
        }
        return ArrayView(data() + (size() - count), count);
    }

    // section() used to be sub()

    ArrayView section(size_type offset, size_type count) PRIME_NOEXCEPT
    {
        if (!PRIME_DEBUG_GUARD(offset <= size())) {
            offset = size();
        }
        if (!PRIME_DEBUG_GUARD(offset + count <= size())) {
            count = size() - offset;
        }

        return ArrayView(data() + offset, count);
    }

    ArrayView section(size_type offset) PRIME_NOEXCEPT
    {
        pointer base;
        size_type count;

        if (PRIME_DEBUG_GUARD(offset <= size())) {
            base = data() + offset;
            count = size() - offset;
        } else {
            base = nullptr;
            count = 0;
        }

        return ArrayView(base, count);
    }

private:
    pointer _array;
    size_type _length;
};

template <typename Element>
inline ArrayView<Element> MakeArrayView(const ArrayView<Element>& copy) PRIME_NOEXCEPT
{
    return ArrayView<Element>(copy.data(), copy.size());
}

template <typename Element>
inline ArrayView<Element> MakeArrayView(Element* array, size_t length) PRIME_NOEXCEPT
{
    return ArrayView<Element>(array, length);
}

template <typename Element>
inline ArrayView<Element> MakeArrayView(Element* begin, Element* end) PRIME_NOEXCEPT
{
    return ArrayView<Element>(begin, static_cast<size_t>(end - begin));
}

#ifndef PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES
template <typename Element, size_t autoArraySize>
inline ArrayView<Element> MakeArrayView(Element (&array)[autoArraySize]) PRIME_NOEXCEPT
{
    return ArrayView<Element>(&array[0], autoArraySize);
}
#endif

#if !PRIME_MSC_AND_OLDER(1300)
/// The second parameter is just to ensure we have a container.
template <typename Container>
inline ArrayView<const typename Container::value_type> MakeArrayView(const Container& container, typename Container::const_iterator* = NULL) PRIME_NOEXCEPT
{
    return ArrayView<const typename Container::value_type>(&container[0], container.size());
}

/// The second parameter is just to ensure we have a container.
template <typename Container>
inline ArrayView<typename Container::value_type> MakeArrayView(Container& container, typename Container::const_iterator* = NULL) PRIME_NOEXCEPT
{
    return ArrayView<typename Container::value_type>(&container[0], container.size());
}
#endif

#ifdef PRIME_COMPILER_INITLIST
template <typename Element>
inline ArrayView<Element> MakeArrayView(std::initializer_list<Element> list) PRIME_NOEXCEPT
{
    return ArrayView<Element>(list.begin(), list.size());
}
#endif

//
// ostream
//

#ifdef PRIME_ENABLE_IOSTREAMS
template <typename Element, typename OStreamTraits>
std::basic_ostream<Element, OStreamTraits>& operator<<(std::basic_ostream<Element, OStreamTraits>& os,
    const ArrayView<Element>& array)
{
    for (size_t i = 0; i != array.size(); ++i) {
        os << array[i];
    }
    return os;
}
#endif
}

#endif
