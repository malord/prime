// Copyright 2000-2021 Mark H. P. Lord

#include "Data.h"
#include "TextEncoding.h"

namespace Prime {

//
// Data
//

Data::Data()
{
}

Data::Data(size_type count, unsigned char value)
    : _data(count, static_cast<char>(value))
{
}

Data::Data(size_type count)
    : _data(count, 0)
{
}

Data::Data(const void* begin, const void* end)
    : _data(reinterpret_cast<const char*>(begin), reinterpret_cast<const char*>(end))
{
}

Data::Data(const void* bytes, size_type numberOfBytes)
    : _data(reinterpret_cast<const char*>(bytes), numberOfBytes)
{
}

Data::Data(StringView string)
    : _data(string.begin(), string.end())
{
}

Data& Data::operator=(StringView string)
{
    _data.assign(string.begin(), string.end());
    return *this;
}

Data::Data(const char* string)
    : _data(string)
{
}

Data& Data::operator=(const char* string)
{
    _data.assign(string);
    return *this;
}

Data::Data(const std::string& string)
    : _data(string)
{
}

Data& Data::operator=(const std::string& string)
{
    _data.assign(string);
    return *this;
}

#ifdef PRIME_COMPILER_RVALUEREF
Data::Data(std::string&& string)
    : _data(std::move(string))
{
}

Data& Data::operator=(std::string&& string)
{
    _data = std::move(string);
    return *this;
}
#endif

Data::Data(const Data& copy)
    : _data(copy._data)
{
}

Data& Data::operator=(const Data& rhs)
{
    _data = rhs._data;
    return *this;
}

#ifdef PRIME_COMPILER_RVALUEREF
Data::Data(Data&& move)
{
    _data.swap(move._data);
}

Data& Data::operator=(Data&& rhs)
{
    _data.swap(rhs._data);
    return *this;
}
#endif

void Data::assign(size_type count, unsigned char value)
{
    _data.assign(count, value);
}

void Data::assign(const void* begin, const void* end)
{
    _data.assign(reinterpret_cast<const char*>(begin), reinterpret_cast<const char*>(end));
}

void Data::assign(const void* bytes, size_type numberOfBytes)
{
    _data.assign(reinterpret_cast<const char*>(bytes), numberOfBytes);
}

void Data::reserve(size_type newCapacity)
{
    _data.reserve(newCapacity);
}

void Data::shrink_to_fit()
{
#ifdef PRIME_CXX11_STL
    _data.shrink_to_fit();
#else
    std::string temp(_data.c_str(), _data.size());
    _data.swap(temp);
#endif
}

void Data::clear()
{
    _data.clear();
}

Data::iterator Data::insert(iterator pos, unsigned char value)
{
    return _data.insert(pos, static_cast<char>(value));
}

#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
Data::iterator Data::insert(const_iterator pos, unsigned char value)
{
    return _data.insert(pos, static_cast<char>(value));
}
#endif

void Data::insert(iterator pos, size_type count, unsigned char value)
{
    _data.insert(pos, count, static_cast<char>(value));
}

#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
void Data::insert(const_iterator pos, size_type count, unsigned char value)
{
    _data.insert(pos, count, static_cast<char>(value));
}
#endif

void Data::insert(iterator pos, const void* begin, const void* end)
{
    _data.insert(pos, reinterpret_cast<const char*>(begin), reinterpret_cast<const char*>(end));
}

#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
void Data::insert(const_iterator pos, const void* begin, const void* end)
{
    _data.insert(pos, reinterpret_cast<const char*>(begin), reinterpret_cast<const char*>(end));
}
#endif

void Data::insert(iterator pos, const_iterator begin, const_iterator end)
{
    _data.insert(pos, begin, end);
}

#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
void Data::insert(const_iterator pos, const_iterator begin, const_iterator end)
{
    _data.insert(pos, begin, end);
}
#endif

Data::iterator Data::erase(iterator pos)
{
    return _data.erase(pos);
}

#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
Data::iterator Data::erase(const_iterator pos)
{
    return _data.erase(pos);
}
#endif

Data::iterator Data::erase(iterator first, iterator last)
{
    return _data.erase(first, last);
}

#ifndef PRIME_COMPILER_NO_CONST_ITER_CAST
Data::iterator Data::erase(const_iterator first, const_iterator last)
{
    return _data.erase(first, last);
}
#endif

void Data::resize(size_type count, unsigned char value)
{
    _data.resize(count, static_cast<char>(value));
}

void Data::swap(Data& data)
{
    _data.swap(data._data);
}

void Data::swap(std::string& string)
{
    _data.swap(string);
}

//
// Data conversions
//

bool UnsafeConvert(Data& output, StringView input)
{
    Data data;
    if (!Base64DecodeAppend(data.string(), input)) {
        return false;
    }

    output.swap(data);
    return true;
}

bool StringAppend(std::string& output, const Data& value)
{
    Base64EncodeAppend(output, &value[0], value.size());
    return true;
}

}
