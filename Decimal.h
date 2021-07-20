// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DECIMAL_H
#define PRIME_DECIMAL_H

#include "Config.h"
#include "StringView.h"
#include "Value.h"
#ifdef PRIME_ENABLE_IOSTREAMS
#include <ostream>
#endif

namespace Prime {

/// A decimal number with 19 digits of precision.
class PRIME_PUBLIC Decimal {
public:
    static Optional<Decimal> fromString(StringView string) PRIME_NOEXCEPT;

    static bool parse(Decimal& decimal, StringView string) PRIME_NOEXCEPT;

    static Decimal parseOrZero(StringView string) PRIME_NOEXCEPT
    {
        return parseOr(string, Decimal(0));
    }

    static Decimal parseOr(StringView string, const Decimal& orElse) PRIME_NOEXCEPT;

    static bool insertThousandSeparators(char* buffer, size_t bufferSize, char separator);

    Decimal() PRIME_NOEXCEPT
    {
        _num.sign = 0;
        _num.approx = 0;
        _num.e = 0;
        _num.m = 0;
    }

    Decimal(const Decimal& copy) PRIME_NOEXCEPT : _num(copy._num)
    {
    }

    Decimal(double from) PRIME_NOEXCEPT;

    Decimal(int from) PRIME_NOEXCEPT;

    Decimal(int64_t from) PRIME_NOEXCEPT;

    bool isZero() const PRIME_NOEXCEPT { return operator==(Decimal(0)); }

    int toInt() const PRIME_NOEXCEPT;

    bool toInt(int& out) const PRIME_NOEXCEPT;

    int32_t toInt32() const PRIME_NOEXCEPT;

    bool toInt32(int32_t& out) const PRIME_NOEXCEPT;

    int64_t toInt64() const PRIME_NOEXCEPT;

    bool toInt64(int64_t& out) const PRIME_NOEXCEPT;

    double toDouble() const PRIME_NOEXCEPT;

    bool toDouble(double& out) const PRIME_NOEXCEPT;

    enum RoundMode {
        RoundModeHalfAwayFromZero,
        RoundModeHalfToEven,
        RoundModeTowardsZero,
        RoundModeBankersRounding = RoundModeHalfToEven
    };

    Decimal getRounded(int digits, RoundMode roundMode = RoundModeHalfAwayFromZero) const PRIME_NOEXCEPT;

    std::string toString() const;

    std::string toString(int digits, RoundMode roundMode = RoundModeHalfAwayFromZero) const;

    std::string toStringWithThousandSeparator(char separator) const;

    std::string toStringWithThousandSeparator(char separator, int digits,
        RoundMode roundMode = RoundModeHalfAwayFromZero) const;

    bool toString(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT;

    bool toString(char* buffer, size_t bufferSize, int digits, RoundMode roundMode = RoundModeHalfAwayFromZero) const PRIME_NOEXCEPT;

    bool toStringWithThousandSeparator(char* buffer, size_t bufferSize, char separator) const PRIME_NOEXCEPT;

    bool toStringWithThousandSeparator(char* buffer, size_t bufferSize, char separator, int digits,
        RoundMode roundMode = RoundModeHalfAwayFromZero) const PRIME_NOEXCEPT;

    bool isInfinite() const PRIME_NOEXCEPT;

    bool isNaN() const PRIME_NOEXCEPT;

    bool isReal() const PRIME_NOEXCEPT { return !isInfinite() && !isNaN(); }

    int compare(const Decimal& rhs) const PRIME_NOEXCEPT;

    Decimal operator+(const Decimal& rhs) const PRIME_NOEXCEPT;
    Decimal operator-(const Decimal& rhs) const PRIME_NOEXCEPT;
    Decimal operator*(const Decimal& rhs) const PRIME_NOEXCEPT;
    Decimal operator/(const Decimal& rhs) const PRIME_NOEXCEPT;

    Decimal& operator+=(const Decimal& rhs) PRIME_NOEXCEPT;
    Decimal& operator-=(const Decimal& rhs) PRIME_NOEXCEPT;
    Decimal& operator*=(const Decimal& rhs) PRIME_NOEXCEPT;
    Decimal& operator/=(const Decimal& rhs) PRIME_NOEXCEPT;

    bool operator==(const Decimal& rhs) const PRIME_NOEXCEPT { return compare(rhs) == 0; }
    bool operator!=(const Decimal& rhs) const PRIME_NOEXCEPT { return compare(rhs) != 0; }
    bool operator<(const Decimal& rhs) const PRIME_NOEXCEPT { return compare(rhs) < 0; }
    bool operator<=(const Decimal& rhs) const PRIME_NOEXCEPT { return compare(rhs) <= 0; }
    bool operator>(const Decimal& rhs) const PRIME_NOEXCEPT { return compare(rhs) > 0; }
    bool operator>=(const Decimal& rhs) const PRIME_NOEXCEPT { return compare(rhs) >= 0; }

    bool operator!() const PRIME_NOEXCEPT { return operator==(Decimal(0)); }

private:
    struct Number {
        unsigned char sign;
        unsigned char approx;
        short e;
        uint64_t m;
    };

    Decimal(const Number& number) PRIME_NOEXCEPT : _num(number)
    {
    }

    mutable Number _num;

    static void adjustExponent(Number& a, Number& b) PRIME_NOEXCEPT;

    static Number add(Number lhs, Number rhs) PRIME_NOEXCEPT;

    static Number subtract(Number lhs, Number rhs) PRIME_NOEXCEPT;

    static Number multiply(Number lhs, Number rhs) PRIME_NOEXCEPT;

    static Number divide(Number lhs, Number rhs) PRIME_NOEXCEPT;

    static bool isInfinite(Number number) PRIME_NOEXCEPT;

    static bool isNaN(Number number) PRIME_NOEXCEPT;

    enum CompareResult {
        CompareLess,
        CompareEqual,
        CompareGreater,
        CompareIncomparable
    };
    static CompareResult compare(Number lhs, Number rhs) PRIME_NOEXCEPT;

    static Number round(Number x, int digits) PRIME_NOEXCEPT;

    static Number roundHalfToEven(Number x, int digits) PRIME_NOEXCEPT;

    static Number roundTowardsZero(Number x, int digits) PRIME_NOEXCEPT;

    static Number parse(const char* in, const char* end, bool allowExponent) PRIME_NOEXCEPT;

    static Number fromInt64(int64_t from) PRIME_NOEXCEPT;

    static Number fromDouble(double from) PRIME_NOEXCEPT;

    static int32_t toInt32(Number number, bool& lossy) PRIME_NOEXCEPT;

    static bool toDouble(Number number, double& output) PRIME_NOEXCEPT;

    static int64_t toInt64(Number number, bool& lossy) PRIME_NOEXCEPT;

    static int toString(Number x, char* zOut, bool alwaysAppendFraction) PRIME_NOEXCEPT;
};

//
// Conversions
//

PRIME_PUBLIC bool StringAppend(std::string& string, const Decimal& decimal);

PRIME_PUBLIC std::string ToStringEmptyIfZero(const Decimal& decimal);

PRIME_PUBLIC bool UnsafeConvert(Decimal& decimal, StringView input) PRIME_NOEXCEPT;

PRIME_PUBLIC bool UnsafeConvert(Decimal& decimal, const Value& input);

PRIME_PUBLIC bool UnsafeConvert(Value& output, const Decimal& decimal);

inline bool UnsafeConvert(Decimal& decimal, const char* input) PRIME_NOEXCEPT
{
    return UnsafeConvert(decimal, StringView(input));
}

inline bool UnsafeConvert(Decimal& decimal, const std::string& input) PRIME_NOEXCEPT
{
    return UnsafeConvert(decimal, StringView(input));
}

template <typename Input>
Decimal ToDecimal(const Input& input, const Decimal& defaultValue = Decimal()) PRIME_NOEXCEPT
{
    Decimal result;
    if (!UnsafeConvert(result, input)) {
        result = defaultValue;
    }
    return result;
}

//
// ostream support
//

#ifdef PRIME_ENABLE_IOSTREAMS

// Must be in the header since Core doesn't build with PRIME_ENABLE_IOSTREAMS by default.
inline std::ostream& operator<<(std::ostream& os, const Decimal& decimal)
{
    // TODO: use precision state
    char buf[128];
    decimal.toString(buf, sizeof(buf));
    return os << buf;
}

#endif
}

#endif
