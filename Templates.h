// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TEMPLATES_H
#define PRIME_TEMPLATES_H

#include "Config.h"
#include <algorithm>
#include <set>
#include <vector>

namespace Prime {

//
// ImpliedComparisonOperators
//

/// Provides !=, >, >= and <= for classes which implement == and <.
/// Usage: class MyClass : public ImpliedComparisonOperators<MyClass>
template <typename Class>
class ImpliedComparisonOperators {
public:
    bool operator!=(const Class& rhs) const
    {
        return !static_cast<const Class*>(this)->operator==(rhs);
    }

    bool operator>(const Class& rhs) const
    {
        return rhs.operator<(*static_cast<const Class*>(this));
    }

    bool operator<=(const Class& rhs) const
    {
        return !rhs.operator<(*static_cast<const Class*>(this));
    }

    bool operator>=(const Class& rhs) const
    {
        return !static_cast<const Class*>(this)->operator<(rhs);
    }
};

//
// Template magic/evil
//

// IsSameType and EnableIf have moved to Common.h

template <typename Base, typename Derived>
struct Host {
    operator Base*() const;
    operator Derived*();
};

template <typename Base, typename Derived>
struct IsBaseOf {
    typedef char (&Yes)[1];
    typedef char (&No)[2];

    template <typename Type>
    static Yes override(Derived*, Type);
    static No override(Base*, int);

    PRIME_STATIC_CONST_BOOL(value, sizeof(override(Host<Base, Derived>(), int())) == sizeof(Yes));
};

template <typename Dummy, typename T>
struct EnableIfTypeExists {
    typedef T Type;
};

template <typename Base, typename Derived, typename MemberType, MemberType Derived::*MemberPointer>
MemberType& DerivedMemberPointer(Base& base)
{
    return static_cast<Derived&>(base).*MemberPointer;
}

//
// Safe delete - delete the object and null the pointer
//

template <typename Type>
inline void SafeDelete(Type*& pointer) PRIME_NOEXCEPT
{
    if (pointer) {
        delete pointer;
        pointer = NULL;
    }
}

template <typename Type>
inline void SafeDeleteArray(Type*& pointer) PRIME_NOEXCEPT
{
    if (pointer) {
        delete[] pointer;
        pointer = NULL;
    }
}

//
// Sorting and searching
//

/// An implementation of std::lower_bound that guarantees only to call comp(*it, value) and never
/// comp(value, *it).
template <typename ForwardIterator, typename Type, typename StrictWeakOrdering>
ForwardIterator LowerBound(ForwardIterator first, ForwardIterator last, const Type& value, StrictWeakOrdering comp)
{
#if PRIME_MSC_AND_OLDER(1300)
    typedef ptrdiff_t Distance;
#else
    typedef typename std::iterator_traits<ForwardIterator>::difference_type Distance;
#endif

    Distance len = std::distance(first, last);
    Distance half;
    ForwardIterator middle;

    while (len > 0) {
        half = len / 2;
        middle = first;
        std::advance(middle, half);
        if (comp(*middle, value)) {
            first = middle;
            ++first;
            len = len - half - 1;
        } else {
            len = half;
        }
    }

    return first;
}

/// Returns -1 if left < right, 0 if left == right and 1 if left > right.
template <typename Type>
inline int Compare3Way(const Type& left, const Type& right)
{
    return (left == right) ? 0 : (left < right) ? -1
                                                : 1;
}

/// 3-way lexicographical compare.
template <typename Iter1, typename Iter2>
int LexicographicalCompare3Way(Iter1 begin1, Iter1 end1, Iter2 begin2, Iter2 end2)
{
    for (;;) {
        if (begin1 == end1) {
            return begin2 == end2 ? 0 : -1;
        }

        if (begin2 == end2) {
            return 1;
        }

        if (*begin1 < *begin2) {
            return -1;
        }

        if (*begin2 < *begin1) {
            return 1;
        }

        ++begin1;
        ++begin2;
    }
}

// Returns a pointer to the last element in the sequence which is not equal to value.
template <typename Iterator, typename Value>
Iterator FindLastNot(Iterator from, Iterator to, const Value& value)
{
    while (to > from) {
        --to;
        if (*to == value) {
            break;
        }
    }

    return to;
}

/// Returns the index of the closest element in an array.
template <typename CompareAs, typename Element>
size_t FindClosest(CompareAs value, const Element* values, size_t valuesCount)
{
    PRIME_ASSERT(valuesCount != 0);

    size_t closestIndex = 0;
    CompareAs closestDiff = (CompareAs)values[0] - (CompareAs)value;
    if (closestDiff < 0) {
        closestDiff = -closestDiff;
    }
    for (size_t index = 1; index != valuesCount; ++index) {
        CompareAs diff = (CompareAs)values[index] - (CompareAs)value;
        if (diff < 0) {
            diff = -diff;
        }
        if (diff < closestDiff) {
            closestDiff = diff;
            closestIndex = index;
        }
    }

    return closestIndex;
}

/// Given an array of values, find the next (direction > 0) or previous (direction < 0) value in the array after
/// or before the value of current.
template <typename Type>
Type FindNextInDirection(Type current, int direction, const Type* values, size_t valuesCount)
{
    for (size_t i = 0; i != valuesCount; ++i) {
        if (direction < 0) {
            if (values[i] >= current) {
                return values[i ? i - 1 : 0];
            }
        } else {
            if (values[i] > current) {
                return values[i];
            }
        }
    }

    return current;
}

//
// Copying
//

/// std::copy alike that static_casts elements.
template <typename SafeCast, typename InputIterator, typename OutputIterator>
void StaticCastCopy(InputIterator first, InputIterator last, OutputIterator output)
{
    for (; first != last; ++first, ++output) {
        *output = static_cast<SafeCast>(*first);
    }
}

template <typename Dest, typename Src>
void StridedCopy(Dest* dest, ptrdiff_t destStride, const Src* src, ptrdiff_t srcStride, size_t count)
{
    while (count) {
        *dest = *src;
        --count;
        dest = PointerAdd(dest, destStride);
        src = PointerAdd(src, srcStride);
    }
}

template <typename SafeCast, typename Dest, typename Src>
void StridedStaticCastCopy(Dest* dest, ptrdiff_t destStride, const Src* src, ptrdiff_t srcStride, size_t count)
{
    while (count) {
        *dest = static_cast<SafeCast>(*src);
        --count;
        dest = PointerAdd(dest, destStride);
        src = PointerAdd(src, srcStride);
    }
}

template <typename Dest, typename Src>
void StaticCastAssign(Dest& lhs, const Src& rhs)
{
    lhs = static_cast<Dest>(rhs);
}

//
// Container helpers
//

template <typename Type>
void Destroy(Type* begin, Type* end) PRIME_NOEXCEPT
{
    // There used to be a whole hunk of code in here to test whether Type was a POD type and skip this loop,
    // but it was nasty, so now I'm letting the compiler figure out whether this is a no-op.
    for (; begin != end; ++begin) {
        begin->~Type();
    }
}

// TODO: specialise this for std::map, std::set and their unordered equivalents
template <typename Container, typename Value>
typename Container::const_iterator Find(const Container& container, const Value& value)
{
    return std::find(container.begin(), container.end(), value);
}

// TODO: specialise this for std::map, std::set and their unordered equivalents
template <typename Container, typename Value>
typename Container::iterator Find(Container& container, const Value& value)
{
    return std::find(container.begin(), container.end(), value);
}

template <typename Container, typename Value>
bool Contains(const Container& container, const Value& value)
{
    return Find<Container, Value>(container, value) != container.end();
}

template <typename Container, typename Value>
bool SetContains(const Container& container, const Value& value)
{
    return container.find(value) != container.end();
}

template <typename Container, typename Item>
void PushBackUnique(Container& container, PRIME_FORWARDING_REFERENCE(Item) item)
{
    if (!Contains(container, item)) {
        container.push_back(PRIME_FORWARD(Item, item));
    }
}

/// The "remove-erase" idiom (to remove matching items from vectors and lists)
template <typename Container, typename Item>
void RemoveErase(Container& container, const Item& item)
{
    container.erase(std::remove(container.begin(), container.end(), item), container.end());
}

/// Remove items from a set if a predicate says so
template <class T, class Comp, class Alloc, class Predicate>
void DiscardIf(std::set<T, Comp, Alloc>& c, Predicate pred)
{
    for (auto it { c.begin() }, end { c.end() }; it != end;) {
        if (pred(*it)) {
            it = c.erase(it);
        } else {
            ++it;
        }
    }
}

template <typename T, typename OutputContainer = std::vector<T>, typename Operator, typename Container>
OutputContainer Transform(const Container& container, Operator op, typename Container::const_iterator* = NULL)
{
    OutputContainer output;
    std::transform(container.begin(), container.end(), std::back_inserter(output), op);
    return output;
}

template <typename T, typename OutputContainer = std::set<T>, typename Operator, typename Container>
OutputContainer TransformToSet(const Container& container, Operator op, typename Container::const_iterator* = NULL)
{
    OutputContainer output;
    std::transform(container.begin(), container.end(), std::inserter(output, output.begin()), op);
    return output;
}

template <typename T, typename OutputContainer = std::vector<T>, typename Operator, typename Container>
OutputContainer ContainerToVector(const Container& container, typename Container::const_iterator* = NULL)
{
    OutputContainer output;
    std::copy(container.begin(), container.end(), std::back_inserter(output));
    return output;
}

template <typename Container1, typename Container2, typename Output = Container1>
Output ContainerConcat(const Container1& first, const Container2& second)
{
    Output output(first.begin(), first.end());
    output.insert(output.end(), second.begin(), second.end());
    return output;
}

//
// ReverseIterate
//

/// Allows you to iterate backwards in a range based for loop.
template <typename Container>
class ReverseIterateRange {
public:
    /// container must not be a temporary because it will be destructed before the for loop begins
    ReverseIterateRange(const Container& container)
        : _container(&container)
    {
    }

    typename Container::const_reverse_iterator begin() const { return _container->rbegin(); }
    typename Container::const_reverse_iterator end() const { return _container->rend(); }

private:
    const Container* _container;
};

template <typename Container>
ReverseIterateRange<Container> ReverseIterate(const Container& container)
{
    return ReverseIterateRange<Container>(container);
}

//
// DynamicBuffer
//

/// A buffer that is replaced with a dynamic allocation if it's not large enough.
template <typename T, size_t Size>
class DynamicBuffer {
public:
    DynamicBuffer()
        : _ptr(_buffer)
        , _capacity(Size)
    {
    }

    explicit DynamicBuffer(size_t size)
        : _ptr(_buffer)
        , _capacity(Size)
    {
        allocate(size);
    }

    ~DynamicBuffer()
    {
        if (_ptr != _buffer) {
            delete[] _ptr;
        }
    }

    void allocate(size_t size)
    {
        if (_capacity > size) {
            return;
        }

        if (_ptr != _buffer) {
            delete[] _ptr;
        }

        if (size <= Size) {
            _ptr = _buffer;
        } else {
            _ptr = new T[size];
        }

        _capacity = size;
    }

    size_t capacity() const { return _capacity; }

    T* get() const { return _ptr; }

private:
    // TODO: I'd prefer this to be uninitialised memory

    T _buffer[Size];
    T* _ptr;
    size_t _capacity;
};
}

#endif
