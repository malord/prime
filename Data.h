// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DATA_H
#define PRIME_DATA_H

#include "Convert.h"
#include "StringView.h"
#include <vector>

namespace Prime {

/// A container of bytes. Internally the bytes are stored as a std::string but the external interface emulates
/// much of std::vector with the exception that it allows void pointers.
class Data {
public:
    typedef std::string::size_type size_type;
    typedef std::string::iterator iterator;
    typedef std::string::reverse_iterator reverse_iterator;
    typedef std::string::const_iterator const_iterator;
    typedef std::string::const_reverse_iterator const_reverse_iterator;

    typedef unsigned char value_type;

    Data();

    Data(size_type count, value_type value);

    explicit Data(size_type count);

    Data(const void* begin, const void* end);

    Data(const void* bytes, size_type numberOfBytes);

    explicit Data(StringView string);
    Data& operator=(StringView string);

    explicit Data(const char* string);
    Data& operator=(const char* string);

    explicit Data(const std::string& string);
    Data& operator=(const std::string& string);

#ifdef PRIME_COMPILER_RVALUEREF
    explicit Data(std::string&& string);
    Data& operator=(std::string&& string);
#endif

    Data(const Data& copy);
    Data& operator=(const Data& rhs);

#ifdef PRIME_COMPILER_RVALUEREF
    Data(Data&& move);
    Data& operator=(Data&& rhs);
#endif

    void assign(size_type count, value_type value);

    void assign(const void* begin, const void* end);

    void assign(const void* bytes, size_type numberOfBytes);

    template <typename Size>
    const value_type& at(Size pos) const { return reinterpret_cast<const value_type&>(_data.at(pos)); }

    template <typename Size>
    value_type& at(Size pos) { return reinterpret_cast<value_type&>(_data.at(pos)); }

    template <typename Size>
    const value_type& operator[](Size pos) const { return reinterpret_cast<const value_type&>(_data[pos]); }

    template <typename Size>
    value_type& operator[](Size pos) { return reinterpret_cast<value_type&>(_data[pos]); }

    const value_type& front() const { return reinterpret_cast<const value_type&>(_data.front()); }
    value_type& front() { return reinterpret_cast<value_type&>(_data.front()); }

    const value_type& back() const { return reinterpret_cast<const value_type&>(_data.back()); }
    value_type& back() { return reinterpret_cast<value_type&>(_data.back()); }

    const value_type* data() const { return reinterpret_cast<const value_type*>(&_data[0]); }
    value_type* data() { return reinterpret_cast<value_type*>(&_data[0]); }

    const_iterator begin() const { return _data.begin(); }
    const_iterator cbegin() const { return _data.begin(); }
    iterator begin() { return _data.begin(); }

    const_iterator end() const { return _data.end(); }
    const_iterator cend() const { return _data.end(); }
    iterator end() { return _data.end(); }

    const_reverse_iterator rbegin() const { return _data.rbegin(); }
    const_reverse_iterator crbegin() const { return _data.rbegin(); }
    reverse_iterator rbegin() { return _data.rbegin(); }

    const_reverse_iterator rend() const { return _data.rend(); }
    const_reverse_iterator crend() const { return _data.rend(); }
    reverse_iterator rend() { return _data.rend(); }

    bool empty() const { return _data.empty(); }

    size_type size() const { return _data.size(); }

    size_type max_size() const { return _data.max_size(); }

    void reserve(size_type newCapacity);

    size_type capacity() const { return _data.capacity(); }

    void shrink_to_fit();

    void clear();

    iterator insert(iterator pos, value_type value);
#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
    iterator insert(const_iterator pos, value_type value);
#endif

    void insert(iterator pos, size_type count, value_type value);
#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
    void insert(const_iterator pos, size_type count, value_type value);
#endif

    void insert(iterator pos, const void* begin, const void* end);
#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
    void insert(const_iterator pos, const void* begin, const void* end);
#endif

    void insert(iterator pos, const_iterator begin, const_iterator end);
#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
    void insert(const_iterator pos, const_iterator begin, const_iterator end);
#endif

    iterator erase(iterator pos);
#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
    iterator erase(const_iterator pos);
#endif
    iterator erase(iterator first, iterator last);
#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
    iterator erase(const_iterator first, const_iterator last);
#endif

    void push_back(value_type ch)
    {
        _data.push_back(static_cast<char>(ch));
    }
    void emplace_back(value_type ch) { _data.push_back(static_cast<char>(ch)); }

    void pop_back() { _data.pop_back(); }

    void resize(size_type count, value_type value = 0);

    void swap(Data& data);
    void swap(std::string& string);

    const std::string& string() const { return _data; }
    std::string& string() { return _data; }

    StringView view() const { return StringView(_data); }

private:
    std::string _data;
};

inline bool operator==(const Data& lhs, const Data& rhs)
{
    return lhs.string() == rhs.string();
}
inline bool operator!=(const Data& lhs, const Data& rhs)
{
    return lhs.string() != rhs.string();
}
inline bool operator<(const Data& lhs, const Data& rhs)
{
    return lhs.string() < rhs.string();
}
inline bool operator<=(const Data& lhs, const Data& rhs)
{
    return lhs.string() <= rhs.string();
}
inline bool operator>(const Data& lhs, const Data& rhs)
{
    return lhs.string() > rhs.string();
}
inline bool operator>=(const Data& lhs, const Data& rhs)
{
    return lhs.string() >= rhs.string();
}

inline bool operator==(const Data& lhs, StringView rhs)
{
    return lhs.view() == rhs;
}
inline bool operator!=(const Data& lhs, StringView rhs)
{
    return lhs.view() != rhs;
}
inline bool operator<(const Data& lhs, StringView rhs)
{
    return lhs.view() < rhs;
}
inline bool operator<=(const Data& lhs, StringView rhs)
{
    return lhs.view() <= rhs;
}
inline bool operator>(const Data& lhs, StringView rhs)
{
    return lhs.view() > rhs;
}
inline bool operator>=(const Data& lhs, StringView rhs)
{
    return lhs.view() >= rhs;
}

inline bool operator==(StringView lhs, const Data& rhs)
{
    return lhs == rhs.view();
}
inline bool operator!=(StringView lhs, const Data& rhs)
{
    return lhs != rhs.view();
}
inline bool operator<(StringView lhs, const Data& rhs)
{
    return lhs < rhs.view();
}
inline bool operator<=(StringView lhs, const Data& rhs)
{
    return lhs <= rhs.view();
}
inline bool operator>(StringView lhs, const Data& rhs)
{
    return lhs > rhs.view();
}
inline bool operator>=(StringView lhs, const Data& rhs)
{
    return lhs >= rhs.view();
}

/// Don't call this directly, use ToData() or Convert()
PRIME_PUBLIC bool UnsafeConvert(Data& output, StringView input);

/// Don't call this directly, use ToData() or Convert()
inline bool UnsafeConvert(Data& output, const char* input)
{
    return UnsafeConvert(output, StringView(input));
}

/// Don't call this directly, use ToData() or Convert()
inline bool UnsafeConvert(Data& output, const std::string& input)
{
    return UnsafeConvert(output, StringView(input));
}

PRIME_PUBLIC bool StringAppend(std::string& output, const Data& value);

template <typename Input>
Data ToData(const Input& input)
{
    Data result;
    if (!UnsafeConvert(result, input)) {
        result.clear();
    }
    return result;
}
}

#endif
