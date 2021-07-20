// Copyright 2000-2021 Mark H. P. Lord

#include "Convert.h"
#include "StringUtils.h"

namespace Prime {

//
// StringAppend
//

bool StringAppend(std::string& output, bool value)
{
    output += value ? "true" : "false";
    return true;
}

bool StringAppend(std::string& output, char value)
{
    output += value;
    return true;
}

bool StringAppend(std::string& output, signed char value)
{
    output += (char)value;
    return true;
}

bool StringAppend(std::string& output, unsigned char value)
{
    StringAppendFormat(output, "%u", (unsigned int)value);
    return true;
}

bool StringAppend(std::string& output, short value)
{
    StringAppendFormat(output, "%d", (int)value);
    return true;
}

bool StringAppend(std::string& output, unsigned short value)
{
    StringAppendFormat(output, "%u", (unsigned int)value);
    return true;
}

bool StringAppend(std::string& output, int value)
{
    StringAppendFormat(output, "%d", value);
    return true;
}

bool StringAppend(std::string& output, unsigned int value)
{
    StringAppendFormat(output, "%u", value);
    return true;
}

bool StringAppend(std::string& output, long value)
{
    StringAppendFormat(output, "%d", value);
    return true;
}

bool StringAppend(std::string& output, unsigned long value)
{
    StringAppendFormat(output, "%u", value);
    return true;
}

#if defined(ULLONG_MAX)

bool StringAppend(std::string& output, long long value)
{
    // There are portability issues with %lld
    StringAppendFormat(output, "%" PRIdMAX, (intmax_t)value);
    return true;
}

bool StringAppend(std::string& output, unsigned long long value)
{
    // There are portability issues with %llu
    StringAppendFormat(output, "%" PRIuMAX, (intmax_t)value);
    return true;
}

#elif defined(UINT64_MAX)

bool StringAppend(std::string& output, int64_t value)
{
    StringAppendFormat(output, "%" PRId64, (int64_t)value);
    return true;
}

bool StringAppend(std::string& output, uint64_t value)
{
    StringAppendFormat(output, "%" PRIu64, (int64_t)value);
    return true;
}

#endif

bool StringAppend(std::string& output, float value)
{
    StringAppendFormat(output, "%g", value);
    return true;
}

bool StringAppend(std::string& output, double value)
{
    StringAppendFormat(output, "%g", value);
    return true;
}

#ifndef PRIME_LONG_DOUBLE_IS_DOUBLE

bool StringAppend(std::string& output, long double value)
{
    StringAppendFormat(output, "%Lg", value);
    return true;
}

#endif

void QuoteIfNecessary(std::string& out, size_t sizeWas)
{
    if (out.size() == sizeWas) {
        //*out += "\"\"";
    } else {
        std::string::size_type i = out.find_first_of(",\"", sizeWas, 2);
        // If the string contains a comma, a double quote, or the string begins or ends with a space,
        // enclose the whole string in double quotes and double up any double quotes within the string.
        if (i != std::string::npos || out[sizeWas] == ' ' || out[out.size() - 1] == ' ') {
#if 1
            size_t unescapedSize = out.size();
            size_t quoteCount = std::count(out.begin() + sizeWas, out.end(), '"');
            size_t escapedSize = out.size() + quoteCount + 2;
            out.resize(escapedSize);
            std::string::iterator dest = out.end();
            std::string::iterator src = out.begin() + unescapedSize;
            std::string::iterator stop = out.begin() + sizeWas;
            *(--dest) = '"';
            while (src-- != stop) {
                if (*src == '"') {
                    *(--dest) = '"';
                }
                *(--dest) = *src;
            }
            *(--dest) = '"';
#else
            out.insert(sizeWas, 1, '"');
            for (i = sizeWas + 1;; i += 2) {
                i = out.find('"', i);
                if (i == std::string::npos) {
                    break;
                }
                out.insert(i, 1, '"');
            }
            out += '"';
#endif
        }
    }
}

void AppendListSeparator(std::string& output)
{
    output += ", ";
}

bool StringAppend(std::string& output, const std::vector<std::string>& vector)
{
    for (size_t i = 0; i != vector.size(); ++i) {
        if (i != 0) {
            AppendListSeparator(output);
        }

        size_t sizeWas = output.size();
        output += vector[i];
        QuoteIfNecessary(output, sizeWas);
    }

    return true;
}

bool UnsafeConvert(bool& output, StringView input) PRIME_NOEXCEPT
{
    intmax_t n;
    if (UnsafeConvertToInteger(n, input, -1)) {
        output = n != 0;
        return true;
    }

    if (ASCIIEqualIgnoringCase(input, "yes") || ASCIIEqualIgnoringCase(input, "true") || ASCIIEqualIgnoringCase(input, "on")) {
        output = true;
        return true;
    }

    if (ASCIIEqualIgnoringCase(input, "no") || ASCIIEqualIgnoringCase(input, "false") || ASCIIEqualIgnoringCase(input, "off")) {
        output = false;
        return true;
    }

    output = !input.empty();
    return true;
}

bool UnsafeConvert(std::vector<std::string>& output, StringView input, StringView separator, unsigned int flags)
{
    StringSplit(output, input, separator, flags);
    return true;
}

bool StringAppend(std::string& output, const RepeatedStringView& value)
{
    value.appendTo(output);
    return true;
}

bool RepeatedStringView::appendTo(std::string& output) const
{
    output.reserve(output.size() + _count * _string.size());

    for (size_t i = _count; i-- > 0;) {
        if (!StringAppend(output, _string)) {
            return false;
        }
    }

    return true;
}
}
