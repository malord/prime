// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DICTIONARY_H
#define PRIME_DICTIONARY_H

#include "Config.h"
#include <utility>
#include <vector>

namespace Prime {

/// An order preserving map container which doesn't have a mutable operator[], so insertions must be done using
/// the set() or access() methods. The key/value pairs are contained in a contiguous array, maintaining insertion
/// order. The list of key/value pairs is accessible.
template <typename Key, typename Value>
class Dictionary {
public:
    typedef std::pair<Key, Value> value_type;
    typedef std::vector<value_type> vector_type;

    typedef Key key_type;
    typedef Value mapped_type;

    typedef typename vector_type::size_type size_type;
    typedef typename vector_type::difference_type difference_type;
    typedef typename vector_type::iterator iterator;
    typedef typename vector_type::const_iterator const_iterator;
#ifndef PRIME_COMPILER_NO_REVERSE_ITERATOR
    typedef typename vector_type::reverse_iterator reverse_iterator;
    typedef typename vector_type::const_reverse_iterator const_reverse_iterator;
#endif
    typedef typename vector_type::reference reference;
    typedef typename vector_type::const_reference const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;

    Dictionary() PRIME_NOEXCEPT { }

    Dictionary(const Dictionary& copy)
        : _pairs(copy._pairs)
    {
    }

    Dictionary(const value_type& pair)
        : _pairs(1, pair)
    {
    }

    template <typename Iterator>
    Dictionary(Iterator otherBegin, Iterator otherEnd)
        : _pairs(otherBegin, otherEnd)
    {
    }

#ifdef PRIME_COMPILER_RVALUEREF

    Dictionary(Dictionary&& other) PRIME_NOEXCEPT : _pairs(std::move(other._pairs))
    {
    }

#endif

#ifdef PRIME_COMPILER_INITLIST
    Dictionary(std::initializer_list<value_type> list)
        : _pairs(list)
    {
    }
#endif

    ~Dictionary() PRIME_NOEXCEPT
    {
    }

    Dictionary& operator=(const Dictionary& copy)
    {
        _pairs = copy._pairs;
        return *this;
    }

#ifdef PRIME_COMPILER_INITLIST
    Dictionary& operator=(std::initializer_list<value_type> list)
    {
        _pairs.assign(list);
        return *this;
    }
#endif

#ifdef PRIME_COMPILER_RVALUEREF
    Dictionary& operator=(Dictionary&& other) PRIME_NOEXCEPT
    {
        _pairs = std::move(other._pairs);
        return *this;
    }
#endif

    bool empty() const PRIME_NOEXCEPT
    {
        return _pairs.empty();
    }

    size_type size() const PRIME_NOEXCEPT { return _pairs.size(); }

    size_type max_size() const PRIME_NOEXCEPT { return _pairs.max_size(); }

    const_iterator begin() const PRIME_NOEXCEPT { return _pairs.begin(); }
    const_iterator end() const PRIME_NOEXCEPT { return _pairs.end(); }

    iterator begin() PRIME_NOEXCEPT { return _pairs.begin(); }
    iterator end() PRIME_NOEXCEPT { return _pairs.end(); }

    const_iterator cbegin() const PRIME_NOEXCEPT { return _pairs.begin(); }
    const_iterator cend() const PRIME_NOEXCEPT { return _pairs.end(); }

#ifndef PRIME_COMPILER_NO_REVERSE_ITERATOR
    const_reverse_iterator rbegin() const PRIME_NOEXCEPT
    {
        return _pairs.rbegin();
    }
    const_reverse_iterator rend() const PRIME_NOEXCEPT { return _pairs.rend(); }

    reverse_iterator rbegin() PRIME_NOEXCEPT { return _pairs.rbegin(); }
    reverse_iterator rend() PRIME_NOEXCEPT { return _pairs.rend(); }

    const_reverse_iterator crbegin() const PRIME_NOEXCEPT { return _pairs.crbegin(); }
    const_reverse_iterator crend() const PRIME_NOEXCEPT { return _pairs.crend(); }
#endif

    const_pointer data() const PRIME_NOEXCEPT
    {
        return _pairs.data();
    }
    pointer data() PRIME_NOEXCEPT { return _pairs.data(); }

    void clear() PRIME_NOEXCEPT { _pairs.clear(); }

    void reserve(size_type size) { _pairs.reserve(size); }

    template <typename Index>
    const value_type& pair(Index index) const { return _pairs[index]; }

    template <typename Index>
    value_type& pair(Index index) { return _pairs[index]; }

    value_type& front() { return _pairs.front(); }
    const value_type& front() const { return _pairs.front(); }

    value_type& back() { return _pairs.back(); }
    const value_type& back() const { return _pairs.back(); }

    template <typename TempKey>
    const_iterator find(const TempKey& key) const PRIME_NOEXCEPT
    {
        const_iterator i;
        for (i = _pairs.begin(); i != _pairs.end(); ++i) {
            if (i->first == key) {
                break;
            }
        }

        return i;
    }

    template <typename TempKey>
    iterator find(const TempKey& key) PRIME_NOEXCEPT
    {
        iterator i;
        for (i = _pairs.begin(); i != _pairs.end(); ++i) {
            if (i->first == key) {
                break;
            }
        }

        return i;
    }

    template <typename TempKey>
    bool has(const TempKey& key) const PRIME_NOEXCEPT
    {
        return find(key) != _pairs.end();
    }

    template <typename TempKey>
    size_type count(const TempKey& key) const PRIME_NOEXCEPT
    {
        size_type n = 0;
        for (const_iterator i = _pairs.begin(); i != _pairs.end(); ++i) {
            if (i->first == key) {
                ++n;
            }
        }

        return n;
    }

    template <typename TempKey>
    const Value& operator[](const TempKey& key) const PRIME_NOEXCEPT
    {
        const_iterator found = find(key);

        return found == _pairs.end() ? emptyValue : found->second;
    }

    // Intentionally no non-const operator [] - use set() or access():

    template <typename TempKey>
    Value& access(const TempKey& key)
    {
        iterator found = find(key);

        if (found != _pairs.end()) {
            return found->second;
        }

        _pairs.push_back(value_type());
        _pairs.back().first = key;

        return _pairs.back().second;
    }

#ifdef PRIME_COMPILER_RVALUEREF
    Value& access(Key&& key)
    {
        iterator found = find(key);

        if (found != _pairs.end()) {
            return found->second;
        }

        _pairs.push_back(value_type(std::move(key), emptyValue));
        return _pairs.back().second;
    }
#endif

    std::pair<iterator, bool> insert(const value_type& pair)
    {
        iterator found = find(pair.key);

        if (found != _pairs.end()) {
            found->second = pair.second;
            return std::pair<iterator, bool>(found, false);
        }

        _pairs.push_back(pair);
        return std::pair<iterator, bool>(_pairs.back().end() - 1, true);
    }

#ifdef PRIME_COMPILER_RVALUEREF
    std::pair<iterator, bool> insert(value_type&& pair)
    {
        iterator found = find(pair.first);

        if (found != _pairs.end()) {
            found->second = PRIME_MOVE(pair.second);
            return std::pair<iterator, bool>(found, false);
        }

        _pairs.push_back(PRIME_MOVE(pair));
        return std::pair<iterator, bool>(_pairs.end() - 1, true);
    }
#endif

    // TODO: insert(begin, end)
    // TODO: insert(initializer_list)

    template <typename TempKey>
    const Value& get(const TempKey& key) const PRIME_NOEXCEPT
    {
        const_iterator found = find(key);

        return found == _pairs.end() ? emptyValue : found->second;
    }

    template <typename TempKey>
    void set(const TempKey& key, const Value& value)
    {
        access(key) = value;
    }

#ifdef PRIME_COMPILER_RVALUEREF
    template <typename TempKey>
    void set(const TempKey& key, Value&& value)
    {
        access(key) = PRIME_MOVE(value);
    }

    void set(Key&& key, Value&& value)
    {
        access(PRIME_MOVE(key)) = PRIME_MOVE(value);
    }

    void set(Key&& key, const Value& value)
    {
        access(PRIME_MOVE(key)) = value;
    }
#endif

    template <typename TempKey>
    bool erase(const TempKey& key)
    {
        iterator found = find(key);

        if (found == _pairs.end()) {
            return false;
        }

        _pairs.erase(found);
        return true;
    }

    void erase(iterator pair)
    {
        _pairs.erase(pair);
    }

    void erase(const_iterator pair)
    {
        _pairs.erase(_pairs.begin() + std::distance(typename vector_type::const_iterator(_pairs.begin()), pair));
    }

    void assign(const Dictionary& other)
    {
        _pairs = other._pairs;
    }

    template <typename Iterator>
    void assign(Iterator otherBegin, Iterator otherEnd)
    {
        _pairs.template assign<Iterator>(otherBegin, otherEnd);
    }

#ifdef PRIME_COMPILER_INITLIST
    Dictionary& assign(std::initializer_list<value_type> list)
    {
        _pairs.assign(list);
        return *this;
    }
#endif

    template <typename Array>
    void assign(const Array& array, typename Array::const_iterator* = NULL)
    {
        _pairs.assign(array.begin(), array.end());
    }

    void move(Dictionary& other) { _pairs.swap(other._pairs); }

    Dictionary& push_back(const value_type& pair)
    {
        _pairs.push_back(pair);
        return *this;
    }

#ifdef PRIME_COMPILER_RVALUEREF
    Dictionary& push_back(value_type&& pair)
    {
        _pairs.push_back(std::move(pair));
        return *this;
    }
#endif

    void swap(Dictionary& other)
    {
        _pairs.swap(other._pairs);
    }

    bool operator==(const Dictionary& other) const { return _pairs == other._pairs; }
    bool operator<(const Dictionary& other) const { return _pairs < other._pairs; }

    PRIME_IMPLIED_COMPARISONS_OPERATORS(const Dictionary&);

protected:
    template <typename Type, typename TempKey>
    static Type find(Type pairsBegin, Type pairsEnd, const TempKey& key) PRIME_NOEXCEPT
    {
        for (; pairsBegin != pairsEnd; ++pairsBegin) {
            if (pairsBegin->first == key) {
                return pairsBegin;
            }
        }

        return pairsEnd;
    }

    vector_type _pairs;
    static Value emptyValue;

    // The plan was to augment the vector with a hashtable at some point, but dictionary lookups have never shown
    // up in the profiler.
};

template <typename Key, typename Value>
Value Dictionary<Key, Value>::emptyValue;

template <typename Key, typename Value>
void swap(Dictionary<Key, Value>& a, Dictionary<Key, Value>& b)
{
    a.swap(b);
}
}

#endif
