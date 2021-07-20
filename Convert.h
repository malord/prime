// Copyright 2000-2021 Mark H. P. Lord

//
// Conversions to string (StringAppend, ToString, MakeString, StringJoin)
// Conversions to primitive types (ToBool, ToInt, ToInt64, ToDouble, ToFloat etc.)
//

#ifndef PRIME_CONVERT_H
#define PRIME_CONVERT_H

#include "Array.h"
#include "ArrayView.h"
#include "NumberParsing.h"
#include "Optional.h"
#include "StringUtils.h"
#include "StringView.h"
#include <string>
#include <vector>
#ifdef PRIME_CXX11_STL
#include <functional>
#endif
#include "Optional.h"

namespace Prime {

//
// StringAppend and MakeString
// Write a StringAppend overload for your type to add MakeString/StringAppend(arg1, arg2, ...)/ToString support.
// Fundamental overloads of StringAppend (i.e., appending strings to strings) are in StringUtils.h
//

PRIME_PUBLIC bool StringAppend(std::string& output, bool value);
PRIME_PUBLIC bool StringAppend(std::string& output, char value);
PRIME_PUBLIC bool StringAppend(std::string& output, signed char value);
PRIME_PUBLIC bool StringAppend(std::string& output, unsigned char value);
PRIME_PUBLIC bool StringAppend(std::string& output, short value);
PRIME_PUBLIC bool StringAppend(std::string& output, unsigned short value);
PRIME_PUBLIC bool StringAppend(std::string& output, int value);
PRIME_PUBLIC bool StringAppend(std::string& output, unsigned int value);
PRIME_PUBLIC bool StringAppend(std::string& output, long value);
PRIME_PUBLIC bool StringAppend(std::string& output, unsigned long value);

#if defined(ULLONG_MAX)
PRIME_PUBLIC bool StringAppend(std::string& output, long long value);
PRIME_PUBLIC bool StringAppend(std::string& output, unsigned long long value);
#elif defined(UINT64_MAX)
PRIME_PUBLIC bool StringAppend(std::string& output, int64_t value);
PRIME_PUBLIC bool StringAppend(std::string& output, uint64_t value);
#endif

PRIME_PUBLIC bool StringAppend(std::string& output, float value);
PRIME_PUBLIC bool StringAppend(std::string& output, double value);

#ifndef PRIME_LONG_DOUBLE_IS_DOUBLE
PRIME_PUBLIC bool StringAppend(std::string& output, long double value);
#endif

PRIME_PUBLIC bool StringAppend(std::string& output, const std::vector<std::string>& vector);

template <typename Type>
bool StringAppend(std::string& output, const Optional<Type>& optional)
{
    if (optional.has_value()) {
        return StringAppend(output, optional.value());
    } else {
        return true;
    }
}

PRIME_PUBLIC void QuoteIfNecessary(std::string& out, size_t sizeWas);

PRIME_PUBLIC void AppendListSeparator(std::string& output);

template <typename T>
inline bool StringAppend(std::string& output, const T& container, typename T::const_iterator* = NULL)
{
    typename T::const_iterator begin = container.begin();
    typename T::const_iterator end = container.end();

    bool first = true;
    for (typename T::const_iterator iter = begin; iter != end; ++iter) {
        if (first) {
            first = false;
        } else {
            AppendListSeparator(output);
        }

        size_t sizeWas = output.size();
        StringAppend(output, *iter);
        QuoteIfNecessary(output, sizeWas);
    }

    return true;
}

#ifdef PRIME_COMPILER_VARIADIC_TEMPLATES

inline bool StringAppend(std::string&)
{
    return true;
}

template <typename T1, typename T2, typename... T3>
bool StringAppend(std::string& output, T1&& t1, T2&& t2, T3&&... t3)
{
    return StringAppend(output, std::forward<T1>(t1)) && StringAppend(output, std::forward<T2>(t2)) && StringAppend(output, std::forward<T3>(t3)...);
}

template <typename T1, typename... T3>
std::string MakeString(T1&& t1, T3&&... t3)
{
    std::string result;
    StringAppend(result, std::forward<T1>(t1));
    StringAppend(result, std::forward<T3>(t3)...);
    return result;
}

#else

#define PRIMESTRU_TYPENAMES1 typename T1
#define PRIMESTRU_TYPENAMES2 PRIMESTRU_TYPENAMES1, typename T2
#define PRIMESTRU_TYPENAMES3 PRIMESTRU_TYPENAMES2, typename T3
#define PRIMESTRU_TYPENAMES4 PRIMESTRU_TYPENAMES3, typename T4
#define PRIMESTRU_TYPENAMES5 PRIMESTRU_TYPENAMES4, typename T5
#define PRIMESTRU_TYPENAMES6 PRIMESTRU_TYPENAMES5, typename T6
#define PRIMESTRU_TYPENAMES7 PRIMESTRU_TYPENAMES6, typename T7
#define PRIMESTRU_TYPENAMES8 PRIMESTRU_TYPENAMES7, typename T8
#define PRIMESTRU_TYPENAMES9 PRIMESTRU_TYPENAMES8, typename T9
#define PRIMESTRU_TYPENAMES10 PRIMESTRU_TYPENAMES9, typename T10
#define PRIMESTRU_TYPENAMES11 PRIMESTRU_TYPENAMES10, typename T11
#define PRIMESTRU_TYPENAMES12 PRIMESTRU_TYPENAMES11, typename T12
#define PRIMESTRU_TYPENAMES13 PRIMESTRU_TYPENAMES12, typename T13
#define PRIMESTRU_TYPENAMES14 PRIMESTRU_TYPENAMES13, typename T14
#define PRIMESTRU_TYPENAMES15 PRIMESTRU_TYPENAMES14, typename T15
#define PRIMESTRU_TYPENAMES16 PRIMESTRU_TYPENAMES15, typename T16

#define PRIMESTRU_PARAMS1 const T1& t1
#define PRIMESTRU_PARAMS2 PRIMESTRU_PARAMS1, const T2& t2
#define PRIMESTRU_PARAMS3 PRIMESTRU_PARAMS2, const T3& t3
#define PRIMESTRU_PARAMS4 PRIMESTRU_PARAMS3, const T4& t4
#define PRIMESTRU_PARAMS5 PRIMESTRU_PARAMS4, const T5& t5
#define PRIMESTRU_PARAMS6 PRIMESTRU_PARAMS5, const T6& t6
#define PRIMESTRU_PARAMS7 PRIMESTRU_PARAMS6, const T7& t7
#define PRIMESTRU_PARAMS8 PRIMESTRU_PARAMS7, const T8& t8
#define PRIMESTRU_PARAMS9 PRIMESTRU_PARAMS8, const T9& t9
#define PRIMESTRU_PARAMS10 PRIMESTRU_PARAMS9, const T10& t10
#define PRIMESTRU_PARAMS11 PRIMESTRU_PARAMS10, const T11& t11
#define PRIMESTRU_PARAMS12 PRIMESTRU_PARAMS11, const T12& t12
#define PRIMESTRU_PARAMS13 PRIMESTRU_PARAMS12, const T13& t13
#define PRIMESTRU_PARAMS14 PRIMESTRU_PARAMS13, const T14& t14
#define PRIMESTRU_PARAMS15 PRIMESTRU_PARAMS14, const T15& t15
#define PRIMESTRU_PARAMS16 PRIMESTRU_PARAMS15, const T16& t16

#define PRIMESTRU_APPENDSTRINGS1 StringAppend(result, t1);
#define PRIMESTRU_APPENDSTRINGS2 PRIMESTRU_APPENDSTRINGS1 StringAppend(result, t2);
#define PRIMESTRU_APPENDSTRINGS3 PRIMESTRU_APPENDSTRINGS2 StringAppend(result, t3);
#define PRIMESTRU_APPENDSTRINGS4 PRIMESTRU_APPENDSTRINGS3 StringAppend(result, t4);
#define PRIMESTRU_APPENDSTRINGS5 PRIMESTRU_APPENDSTRINGS4 StringAppend(result, t5);
#define PRIMESTRU_APPENDSTRINGS6 PRIMESTRU_APPENDSTRINGS5 StringAppend(result, t6);
#define PRIMESTRU_APPENDSTRINGS7 PRIMESTRU_APPENDSTRINGS6 StringAppend(result, t7);
#define PRIMESTRU_APPENDSTRINGS8 PRIMESTRU_APPENDSTRINGS7 StringAppend(result, t8);
#define PRIMESTRU_APPENDSTRINGS9 PRIMESTRU_APPENDSTRINGS8 StringAppend(result, t9);
#define PRIMESTRU_APPENDSTRINGS10 PRIMESTRU_APPENDSTRINGS9 StringAppend(result, t10);
#define PRIMESTRU_APPENDSTRINGS11 PRIMESTRU_APPENDSTRINGS10 StringAppend(result, t11);
#define PRIMESTRU_APPENDSTRINGS12 PRIMESTRU_APPENDSTRINGS11 StringAppend(result, t12);
#define PRIMESTRU_APPENDSTRINGS13 PRIMESTRU_APPENDSTRINGS12 StringAppend(result, t13);
#define PRIMESTRU_APPENDSTRINGS14 PRIMESTRU_APPENDSTRINGS13 StringAppend(result, t14);
#define PRIMESTRU_APPENDSTRINGS15 PRIMESTRU_APPENDSTRINGS14 StringAppend(result, t15);
#define PRIMESTRU_APPENDSTRINGS16 PRIMESTRU_APPENDSTRINGS15 StringAppend(result, t16);

#define PRIMESTRU_MAKESTRING_N(n)                 \
    template <PRIMESTRU_TYPENAMES##n>             \
    std::string MakeString(PRIMESTRU_PARAMS##n)   \
    {                                             \
        std::string result;                       \
        PRIMESTRU_APPENDSTRINGS##n return result; \
    }

PRIMESTRU_MAKESTRING_N(1)
PRIMESTRU_MAKESTRING_N(2)
PRIMESTRU_MAKESTRING_N(3)
PRIMESTRU_MAKESTRING_N(4)
PRIMESTRU_MAKESTRING_N(5)
PRIMESTRU_MAKESTRING_N(6)
PRIMESTRU_MAKESTRING_N(7)
PRIMESTRU_MAKESTRING_N(8)
PRIMESTRU_MAKESTRING_N(9)
PRIMESTRU_MAKESTRING_N(10)
PRIMESTRU_MAKESTRING_N(11)
PRIMESTRU_MAKESTRING_N(12)
PRIMESTRU_MAKESTRING_N(13)
PRIMESTRU_MAKESTRING_N(14)
PRIMESTRU_MAKESTRING_N(15)
PRIMESTRU_MAKESTRING_N(16)

#undef PRIMESTRU_MAKESTRING_N

#define PRIMESTRU_APPENDSTRINGS_N(n)                            \
    template <PRIMESTRU_TYPENAMES##n>                           \
    void StringAppend(std::string& target, PRIMESTRU_PARAMS##n) \
    {                                                           \
        std::string& result = target;                           \
        PRIMESTRU_APPENDSTRINGS##n;                             \
    }

PRIMESTRU_APPENDSTRINGS_N(1)
PRIMESTRU_APPENDSTRINGS_N(2)
PRIMESTRU_APPENDSTRINGS_N(3)
PRIMESTRU_APPENDSTRINGS_N(4)
PRIMESTRU_APPENDSTRINGS_N(5)
PRIMESTRU_APPENDSTRINGS_N(6)
PRIMESTRU_APPENDSTRINGS_N(7)
PRIMESTRU_APPENDSTRINGS_N(8)
PRIMESTRU_APPENDSTRINGS_N(9)
PRIMESTRU_APPENDSTRINGS_N(10)
PRIMESTRU_APPENDSTRINGS_N(11)
PRIMESTRU_APPENDSTRINGS_N(12)
PRIMESTRU_APPENDSTRINGS_N(13)
PRIMESTRU_APPENDSTRINGS_N(14)
PRIMESTRU_APPENDSTRINGS_N(15)
PRIMESTRU_APPENDSTRINGS_N(16)

#endif

template <typename Container>
std::string StringJoin(const Container& container, StringView separator, typename Container::const_iterator* = NULL)
{
    std::string result;
    bool first = true;

    for (typename Container::const_iterator iter = container.begin(); iter != container.end(); ++iter) {
        if (!first) {
            result.insert(result.end(), separator.begin(), separator.end());
        } else {
            first = false;
        }

        StringAppend(result, *iter);
    }

    return result;
}

#ifdef PRIME_CXX11_STL
template <typename Container>
std::string StringJoin(const Container& container, StringView separator,
    std::function<void(std::string&, const typename Container::value_type&)> formatter,
    typename Container::const_iterator* = NULL)
{
    std::string result;
    bool first = true;

    for (typename Container::const_iterator iter = container.begin(); iter != container.end(); ++iter) {
        if (!first) {
            result.insert(result.end(), separator.begin(), separator.end());
        } else {
            first = false;
        }

        formatter(result, *iter);
    }

    return result;
}
#endif

//
// bool UnsafeConvert(out, in)
//
// The conversion functions (ToBool, ToFloat, etc., and Convert) are implemented using the UnsafeConvert functions,
// which can be overridden to support new types. The UnsafeConvert functions should not be called directly as
// they do not guarantee the contents of their output if the conversion fails. Use ToBool, ToFloat, etc or Convert.
// Note that conversions to std::string are handled by overriding StringAppend.
// To override conversions to integer types, override UnsafeConvertToInteger[Array].
// To override conversions to floating point types, override UnsafeConvertToReal[Array].
//

template <typename Input>
inline bool UnsafeConvert(std::string& output, const Input& value)
{
    output.resize(0);
    return StringAppend(output, value);
}

PRIME_PUBLIC bool UnsafeConvert(bool& output, StringView input) PRIME_NOEXCEPT;

inline bool UnsafeConvert(bool& output, const char* input) PRIME_NOEXCEPT
{
    return UnsafeConvert(output, StringView(input));
}

inline bool UnsafeConvert(bool& output, const std::string& input) PRIME_NOEXCEPT
{
    return UnsafeConvert(output, StringView(input));
}

PRIME_PUBLIC bool UnsafeConvert(std::vector<std::string>& output, StringView input,
    StringView separator, unsigned int flags);

inline bool UnsafeConvert(std::vector<std::string>& output, const char* input, StringView separator,
    unsigned int flags)
{
    return UnsafeConvert(output, StringView(input), separator, flags);
}

inline bool UnsafeConvert(std::vector<std::string>& output, const std::string& input,
    StringView separator, unsigned int flags)
{
    return UnsafeConvert(output, StringView(input), separator, flags);
}

template <typename Output>
inline bool UnsafeConvertToInteger(Output& output, StringView input, int base) PRIME_NOEXCEPT
{
    return StringToInt(input, output, base);
}

template <typename Output>
inline bool UnsafeConvertToInteger(Output& output, const char* input, int base) PRIME_NOEXCEPT
{
    return StringToInt(input, output, base);
}

template <typename Output>
inline bool UnsafeConvertToInteger(Output& output, const std::string& input, int base) PRIME_NOEXCEPT
{
    return StringToInt(input.c_str(), output, base);
}

template <typename Output>
bool UnsafeConvertToIntegerArray(ArrayView<Output> array, size_t minCount, StringView input, size_t* count, int base) PRIME_NOEXCEPT
{
    return StringToIntArray(input, array.begin(), minCount, array.size(), count, base);
}

template <typename Output>
bool UnsafeConvertToIntegerArray(ArrayView<Output> array, size_t minCount, const char* input, size_t* count, int base) PRIME_NOEXCEPT
{
    return StringToIntArray(input, array.begin(), minCount, array.size(), count, base);
}

template <typename Output>
bool UnsafeConvertToIntegerArray(ArrayView<Output> array, size_t minCount, const std::string& input, size_t* count, int base) PRIME_NOEXCEPT
{
    return StringToIntArray(input.c_str(), array.begin(), minCount, array.size(), count, base);
}

template <typename Output>
inline bool UnsafeConvertToReal(Output& output, StringView input) PRIME_NOEXCEPT
{
    return StringToReal(input, output);
}

template <typename Output>
inline bool UnsafeConvertToReal(Output& output, const char* input) PRIME_NOEXCEPT
{
    return StringToReal(input, output);
}

template <typename Output>
inline bool UnsafeConvertToReal(Output& output, const std::string& input) PRIME_NOEXCEPT
{
    return StringToReal(input.c_str(), output);
}

template <typename Output>
bool UnsafeConvertToRealArray(ArrayView<Output> array, size_t minCount, StringView input, size_t* count) PRIME_NOEXCEPT
{
    return StringToRealArray(input, array.begin(), minCount, array.size(), count);
}

template <typename Output>
bool UnsafeConvertToRealArray(ArrayView<Output> array, size_t minCount, const char* input, size_t* count) PRIME_NOEXCEPT
{
    return StringToRealArray(input, array.begin(), minCount, array.size(), count);
}

template <typename Output>
bool UnsafeConvertToRealArray(ArrayView<Output> array, size_t minCount, const std::string& input, size_t* count) PRIME_NOEXCEPT
{
    return StringToRealArray(input.c_str(), array.begin(), minCount, array.size(), count);
}

template <typename Input>
inline bool UnsafeConvert(char& output, const Input& input) PRIME_NOEXCEPT
{
    return UnsafeConvertToInteger(output, input, -1);
}

template <typename Input>
inline bool UnsafeConvert(short& output, const Input& input) PRIME_NOEXCEPT
{
    return UnsafeConvertToInteger(output, input, -1);
}

template <typename Input>
inline bool UnsafeConvert(int& output, const Input& input) PRIME_NOEXCEPT
{
    return UnsafeConvertToInteger(output, input, -1);
}

template <typename Input>
inline bool UnsafeConvert(long& output, const Input& input) PRIME_NOEXCEPT
{
    return UnsafeConvertToInteger(output, input, -1);
}

#if defined(ULLONG_MAX)

template <typename Input>
inline bool UnsafeConvert(long long& output, const Input& input) PRIME_NOEXCEPT
{
    return UnsafeConvertToInteger(output, input, -1);
}

#elif defined(UINT64_MAX)

template <typename Input>
inline bool UnsafeConvert(int64_t& output, const Input& input) PRIME_NOEXCEPT
{
    return UnsafeConvertToInteger(output, input, -1);
}
#endif

template <typename Input>
inline bool UnsafeConvert(float& output, const Input& input) PRIME_NOEXCEPT
{
    return UnsafeConvertToReal(output, input);
}

template <typename Input>
inline bool UnsafeConvert(double& output, const Input& input) PRIME_NOEXCEPT
{
    return UnsafeConvertToReal(output, input);
}

template <typename Output, typename Input>
bool ConvertToInteger(Output& output, const Input& input, const Input& defaultValue = Input(), int base = -1) PRIME_NOEXCEPT
{
    if (!UnsafeConvertToInteger(output, input, base)) {
        output = defaultValue;
        return false;
    }

    return true;
}

template <typename Output, typename Input>
bool ConvertToIntegerArray(ArrayView<Output> output, size_t minCount, const Input& input, size_t* count = NULL, int base = -1) PRIME_NOEXCEPT
{
    return UnsafeConvertToIntegerArray(output, minCount, input, count, base);
}

template <typename Output, typename Input>
bool ConvertToIntegerArray(ArrayView<Output> output, const Input& input, int base = -1) PRIME_NOEXCEPT
{
    return UnsafeConvertToIntegerArray(output, output.size(), input, NULL, base);
}

#ifndef PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES

template <typename Output, typename Input, size_t autoOutputSize>
bool ConvertToIntegerArray(Output (&output)[autoOutputSize], size_t minCount, const Input& input, size_t* count = NULL, int base = -1) PRIME_NOEXCEPT
{
    return UnsafeConvertToIntegerArray(ArrayView<Output>(output, autoOutputSize), minCount, input, count, base);
}

template <typename Output, typename Input, size_t autoOutputSize>
bool ConvertToIntegerArray(Output (&output)[autoOutputSize], const Input& input, int base = -1) PRIME_NOEXCEPT
{
    return UnsafeConvertToIntegerArray(ArrayView<Output>(output, autoOutputSize), autoOutputSize, input, NULL, base);
}

#endif

template <typename Output, typename Input>
bool ConvertToReal(Output& output, const Input& input, const Input& defaultValue = Input()) PRIME_NOEXCEPT
{
    if (!UnsafeConvertToReal(output, input)) {
        output = defaultValue;
        return false;
    }

    return true;
}

template <typename Output, typename Input>
bool ConvertToRealArray(ArrayView<Output> output, size_t minCount, const Input& input, size_t* count = NULL) PRIME_NOEXCEPT
{
    return UnsafeConvertToRealArray(output, minCount, input, count);
}

template <typename Output, typename Input>
bool ConvertToRealArray(ArrayView<Output> output, const Input& input) PRIME_NOEXCEPT
{
    return UnsafeConvertToRealArray(output, output.size(), input, NULL);
}

#ifndef PRIME_COMPILER_NO_TEMPLATE_ARRAY_SIZES

template <typename Output, typename Input, size_t autoOutputSize>
bool ConvertToRealArray(Output (&output)[autoOutputSize], size_t minCount, const Input& input, size_t* count = NULL) PRIME_NOEXCEPT
{
    return UnsafeConvertToRealArray(ArrayView<Output>(output, autoOutputSize), minCount, input, count);
}

template <typename Output, typename Input, size_t autoOutputSize>
bool ConvertToRealArray(Output (&output)[autoOutputSize], const Input& input) PRIME_NOEXCEPT
{
    return UnsafeConvertToRealArray(ArrayView<Output>(output, autoOutputSize), autoOutputSize, input, NULL);
}

#endif

//
// Conversions functions (e.g., ToBool, ToFloat)
// Don't override these, override UnsafeConvert instead.
//

template <typename Input>
inline bool ToBool(const Input& input, bool defaultValue = false) PRIME_NOEXCEPT
{
    bool temp;
    return UnsafeConvert(temp, input) ? temp : defaultValue;
}

template <typename Output, typename Input, typename DefaultValue>
inline Output ToInteger(const Input& input, const DefaultValue& defaultValue = DefaultValue(), int base = -1) PRIME_NOEXCEPT
{
    Output temp;
    return UnsafeConvertToInteger(temp, input, base) ? temp : static_cast<Output>(defaultValue);
}

template <typename Input>
inline int ToInt(const Input& input, int defaultValue = 0, int base = -1) PRIME_NOEXCEPT
{
    int temp;
    return UnsafeConvertToInteger(temp, input, base) ? temp : defaultValue;
}

template <typename Input>
inline unsigned int ToUInt(const Input& input, unsigned int defaultValue = 0, int base = -1) PRIME_NOEXCEPT
{
    unsigned int temp;
    return UnsafeConvertToInteger(temp, input, base) ? temp : defaultValue;
}

template <typename Input>
inline int64_t ToInt64(const Input& input, int64_t defaultValue = 0, int base = -1) PRIME_NOEXCEPT
{
    int64_t temp;
    return UnsafeConvertToInteger(temp, input, base) ? temp : defaultValue;
}

#if !PRIME_MSC_AND_OLDER(1300)
template <typename Output, unsigned int ArraySize, typename Input>
Array<Output, ArraySize> ToIntegerArray(const Input& input,
    const Array<Output, ArraySize>& defaultValue = Array<Output, ArraySize>(),
    unsigned int minCount = ArraySize) PRIME_NOEXCEPT
{
    Array<Output, ArraySize> array(defaultValue);
    UnsafeConvertToIntegerArray(ArrayView<Output>(&array[0], array.size()), minCount, input, NULL, -1);
    return array;
}
#else
template <typename Output, unsigned int ArraySize, typename Input>
Array<Output, ArraySize> ToIntegerArray(const Input& input) PRIME_NOEXCEPT
{
    Array<Output, ArraySize> array;
    UnsafeConvertToIntegerArray(ArrayView<Output>(&array[0], array.size()), ArraySize, input, NULL, -1);
    return array;
}
#endif

template <typename Output, typename Input, typename DefaultValue>
inline Output ToReal(const Input& input, const DefaultValue& defaultValue = 0) PRIME_NOEXCEPT
{
    Output temp;
    return UnsafeConvertToReal(temp, input) ? temp : static_cast<Output>(defaultValue);
}

template <typename Input>
inline float ToFloat(const Input& input, float defaultValue = 0) PRIME_NOEXCEPT
{
    float temp;
    return UnsafeConvertToReal(temp, input) ? temp : defaultValue;
}

template <typename Input>
inline double ToDouble(const Input& input, double defaultValue = 0) PRIME_NOEXCEPT
{
    double temp;
    return UnsafeConvertToReal(temp, input) ? temp : defaultValue;
}

#if !PRIME_MSC_AND_OLDER(1300)
template <typename Output, size_t ArraySize, typename Input>
Array<Output, ArraySize> ToRealArray(const Input& input,
    const Array<Output, ArraySize>& defaultValue = Array<Output, ArraySize>(),
    unsigned int minCount = ArraySize) PRIME_NOEXCEPT
{
    Array<Output, ArraySize> array(defaultValue);
    UnsafeConvertToRealArray(ArrayView<Output>(&array[0], array.size()), minCount, input, NULL);
    return array;
}
#else
template <typename Output, size_t ArraySize, typename Input>
Array<Output, ArraySize> ToRealArray(const Input& input,
    const Array<Output, ArraySize>& defaultValue = Array<Output, ArraySize>()) PRIME_NOEXCEPT
{
    Array<Output, ArraySize> array(defaultValue);
    UnsafeConvertToRealArray(ArrayView<Output>(&array[0], array.size()), ArraySize, input, NULL);
    return array;
}
#endif

template <typename Input>
inline std::string ToString(const Input& input, StringView defaultValue)
{
    std::string result;
    if (!StringAppend(result, input)) {
        result.assign(defaultValue.begin(), defaultValue.end());
    }
    return result;
}

template <typename Input>
inline std::string ToString(const Input& input)
{
    std::string result;
    if (!StringAppend(result, input)) {
        result.resize(0);
    }
    return result;
}

/// To support more types, override UnsafeConvert(vector<string>&, input, separator, flags)
template <typename Input>
inline std::vector<std::string> ToStringVector(PRIME_FORWARDING_REFERENCE(Input) input, StringView separator,
    unsigned int flags = 0)
{
    std::vector<std::string> temp;
    if (!UnsafeConvert(temp, PRIME_FORWARD(Input, input), separator, flags)) {
        temp.resize(1);
        temp[0].resize(0);
        if (!StringAppend(temp[0], input)) {
            temp[0].resize(0);
        }
    }

    return temp;
}

//
// Convert
// Prefer this to using UnsafeConvert, which can leave the output uninitialised.
//

template <typename Output, typename Input>
bool Convert(Output& output, PRIME_FORWARDING_REFERENCE(Input) input)
    PRIME_NOEXCEPT_IF(noexcept(UnsafeConvert(output, PRIME_FORWARD(Input, input))))
{
    if (UnsafeConvert(output, PRIME_FORWARD(Input, input))) {
        return true;
    }
    output = Output();
    return false;
}

template <typename Output, typename Input, typename DefaultValue>
bool Convert(Output& output, PRIME_FORWARDING_REFERENCE(Input) input, PRIME_FORWARDING_REFERENCE(DefaultValue) defaultValue)
    PRIME_NOEXCEPT_IF(noexcept(UnsafeConvert(output, PRIME_FORWARD(Input, input))))
{
    if (UnsafeConvert(output, PRIME_FORWARD(Input, input))) {
        return true;
    }
    output = PRIME_FORWARD(DefaultValue, defaultValue);
    return false;
}

template <typename Input>
bool Convert(std::vector<std::string>& output, PRIME_FORWARDING_REFERENCE(Input) input, StringView separator,
    unsigned int flags = 0)
    PRIME_NOEXCEPT_IF(noexcept(UnsafeConvert(output, PRIME_FORWARD(Input, input), separator, flags)))
{
    if (!UnsafeConvert(output, PRIME_FORWARD(Input, input), separator, flags)) {
        output.resize(0);
        return false;
    }

    return true;
}

//
// OptionalConvert
//

/// Returns an Optional instead of a bool
template <typename ToType, typename FromType>
Optional<ToType> OptionalConvert(FromType&& from)
{
    Optional<ToType> result;

    ToType temp;
    if (Convert(temp, from)) {
        result = PRIME_MOVE(temp);
    }

    return result;
}

//
// ConvertContainer/ConvertSet
//

#ifdef PRIME_CXX11_STL

template <typename T, typename OutputContainer = std::vector<T>, typename Container>
OutputContainer ConvertContainer(const Container& container, const T& defaultValue = T(),
    typename Container::const_iterator* = NULL)
{
    OutputContainer output;
    std::transform(container.begin(), container.end(), std::back_inserter(output),
        [&defaultValue](const typename Container::value_type& value) -> T {
            T output;
            Convert(output, value, defaultValue);
            return output;
        });
    return output;
}

/// e.g., ConvertSet<Record::ID>(StringSplit(commaSeparatedIDList, ",", SplitSkipEmpty))
template <typename T, typename OutputSet = std::set<T>, typename Container>
OutputSet ConvertSet(const Container& container, const T& defaultValue = T(),
    typename Container::const_iterator* = NULL)
{
    OutputSet output;
    std::transform(container.begin(), container.end(), std::inserter(output, output.begin()),
        [&defaultValue](const typename Container::value_type& value) -> T {
            T output;
            Convert(output, value, defaultValue);
            return output;
        });
    return output;
}

#endif

//
// RepeatedStringView
//

/// Can be append to a string with StringAppend, MakeString, etc.
class RepeatedStringView {
public:
    RepeatedStringView(size_t count, StringView string)
        : _count(count)
        , _string(string)
    {
    }

    bool appendTo(std::string& output) const;

private:
    size_t _count;
    StringView _string;
};

PRIME_PUBLIC bool StringAppend(std::string& output, const RepeatedStringView& value);
}

#endif
