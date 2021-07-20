// Copyright 2000-2021 Mark H. P. Lord

#include "Decimal.h"
#include "StringUtils.h"

namespace Prime {

// This code is derived from the SQLite 4 decimal code with minor bug fixes and changes to compile as C++

static const short maxExponent = 999;
static const short nanExponent = 2000;

static const uint64_t oneTenthOfMax = UINT64_MAX / 10;

// Adjust the significand and exponent of a and b so that the exponent is the same.
void Decimal::adjustExponent(Number& a, Number& b) PRIME_NOEXCEPT
{
    if (a.e < b.e) {
        adjustExponent(b, a);
        return;
    }
    if (b.m == 0) {
        b.e = a.e;
        return;
    }
    if (a.m == 0) {
        a.e = b.e;
        return;
    }
    if (a.e > b.e + 40) {
        b.approx = 1;
        b.e = a.e;
        b.m = 0;
        return;
    }
    while (a.e > b.e && b.m % 10 == 0) {
        b.m /= 10;
        b.e++;
    }
    while (a.e > b.e && a.m <= oneTenthOfMax) {
        a.m *= 10;
        a.e--;
    }
    while (a.e > b.e) {
        b.m /= 10;
        b.e++;
        b.approx = 1;
    }
}

Decimal::Number Decimal::add(Number lhs, Number rhs) PRIME_NOEXCEPT
{
    uint64_t r;
    if (lhs.sign != rhs.sign) {
        if (lhs.sign) {
            lhs.sign = 0;
            return subtract(rhs, lhs);
        } else {
            rhs.sign = 0;
            return subtract(lhs, rhs);
        }
    }
    if (lhs.e > maxExponent) {
        if (rhs.e > maxExponent && rhs.m == 0)
            return rhs;
        return lhs;
    }
    if (rhs.e > maxExponent) {
        return rhs;
    }
    adjustExponent(lhs, rhs);
    r = lhs.m + rhs.m;
    lhs.approx |= rhs.approx;
    if (r >= lhs.m) {
        lhs.m = r;
    } else {
        if (lhs.approx == 0 && (lhs.m % 10) != 0)
            lhs.approx = 1;
        lhs.m /= 10;
        lhs.e++;
        if (lhs.e > maxExponent)
            return lhs;
        if (lhs.approx == 0 && (rhs.m % 10) != 0)
            lhs.approx = 1;
        lhs.m += rhs.m / 10;
    }
    return lhs;
}

Decimal::Number Decimal::subtract(Number lhs, Number rhs) PRIME_NOEXCEPT
{
    if (lhs.sign != rhs.sign) {
        rhs.sign = lhs.sign;
        return add(lhs, rhs);
    }
    if (lhs.e > maxExponent || rhs.e > maxExponent) {
        lhs.e = nanExponent;
        lhs.m = 0;
        return lhs;
    }
    adjustExponent(lhs, rhs);
    if (rhs.m > lhs.m) {
        Number t = lhs;
        lhs = rhs;
        rhs = t;
        lhs.sign = 1 - lhs.sign;
    }
    lhs.m -= rhs.m;
    lhs.approx |= rhs.approx;
    return lhs;
}

static bool multWillOverflow(uint64_t x, uint64_t y) PRIME_NOEXCEPT
{
    uint64_t xHi, xLo, yHi, yLo;
    xHi = x >> 32;
    yHi = y >> 32;
    if (xHi * yHi)
        return 1;
    xLo = x & 0xffffffff;
    yLo = y & 0xffffffff;
    if ((xHi * yLo + yHi * xLo + (xLo * yLo >> 32)) > 0xffffffff)
        return 1;
    return 0;
}

Decimal::Number Decimal::multiply(Number lhs, Number rhs) PRIME_NOEXCEPT
{
    Number r;

    if (lhs.e > maxExponent || rhs.e > maxExponent) {
        r.sign = lhs.sign ^ rhs.sign;
        r.m = (lhs.m && rhs.m) ? 1 : 0;
        r.e = maxExponent + 1;
        r.approx = 0;
        return r;
    }
    if (lhs.m == 0)
        return lhs;
    if (rhs.m == 0)
        return rhs;
    while (lhs.m % 10 == 0) {
        lhs.m /= 10;
        lhs.e++;
    }
    while (rhs.m % 10 == 0) {
        rhs.m /= 10;
        rhs.e++;
    }
    while (lhs.m % 5 == 0 && rhs.m % 2 == 0) {
        lhs.m /= 5;
        lhs.e++;
        rhs.m /= 2;
    }
    while (rhs.m % 5 == 0 && lhs.m % 2 == 0) {
        rhs.m /= 5;
        rhs.e++;
        lhs.m /= 2;
    }
    r.sign = lhs.sign ^ rhs.sign;
    r.approx = lhs.approx | rhs.approx;
    while (multWillOverflow(lhs.m, rhs.m)) {
        r.approx = 1;
        if (lhs.m > rhs.m) {
            lhs.m /= 10;
            lhs.e++;
        } else {
            rhs.m /= 10;
            rhs.e++;
        }
    }
    r.m = lhs.m * rhs.m;
    r.e = lhs.e + rhs.e;
    return r;
}

Decimal::Number Decimal::divide(Number lhs, Number rhs) PRIME_NOEXCEPT
{
    Number r;
    if (lhs.e > maxExponent) {
        lhs.m = 0;
        return lhs;
    }
    if (rhs.e > maxExponent) {
        if (rhs.m != 0) {
            r.m = 0;
            r.e = 0;
            r.sign = 0;
            r.approx = 1;
            return r;
        }
        return rhs;
    }
    if (rhs.m == 0) {
        r.sign = lhs.sign ^ rhs.sign;
        r.e = nanExponent;
        r.m = 0;
        r.approx = 1;
        return r;
    }
    if (lhs.m == 0) {
        return lhs;
    }
    while (lhs.m < oneTenthOfMax) {
        lhs.m *= 10;
        lhs.e--;
    }
    while (rhs.m % 10 == 0) {
        rhs.m /= 10;
        rhs.e++;
    }
    r.sign = lhs.sign ^ rhs.sign;
    r.approx = lhs.approx | rhs.approx;
    if (r.approx == 0 && lhs.m % rhs.m != 0)
        r.approx = 1;
    r.m = lhs.m / rhs.m;
    r.e = lhs.e - rhs.e;
    return r;
}

bool Decimal::isInfinite(Number number) PRIME_NOEXCEPT
{
    return number.e > maxExponent && number.m != 0;
}

bool Decimal::isNaN(Number number) PRIME_NOEXCEPT
{
    return number.e > maxExponent && number.m == 0;
}

// NaN values are always incompatible
Decimal::CompareResult Decimal::compare(Number lhs, Number rhs) PRIME_NOEXCEPT
{
    if (lhs.e > maxExponent) {
        if (lhs.m == 0)
            return CompareIncomparable;
        if (rhs.e > maxExponent) {
            if (rhs.m == 0)
                return CompareIncomparable;
            if (rhs.sign == lhs.sign)
                return CompareIncomparable;
        }
        return lhs.sign ? CompareLess : CompareGreater;
    }
    if (rhs.e > maxExponent) {
        if (rhs.m == 0)
            return CompareIncomparable;
        return rhs.sign ? CompareGreater : CompareLess;
    }
    if (lhs.sign != rhs.sign) {
        if (lhs.m == 0 && rhs.m == 0)
            return CompareEqual;
        return lhs.sign ? CompareLess : CompareGreater;
    }
    adjustExponent(lhs, rhs);
    if (lhs.sign) {
        Number t = lhs;
        lhs = rhs;
        rhs = t;
    }
    if (lhs.e != rhs.e) {
        return lhs.e < rhs.e ? CompareLess : CompareGreater;
    }
    if (lhs.m != rhs.m) {
        return lhs.m < rhs.m ? CompareLess : CompareGreater;
    }
    return CompareEqual;
}

Decimal::Number Decimal::round(Number x, int N) PRIME_NOEXCEPT
{
    if (N < 0)
        N = 0;
    if (x.e >= -N)
        return x;
    if (x.e < -(N + 30)) {
        memset(&x, 0, sizeof(x));
        return x;
    }
    while (x.e < -(N + 1)) {
        x.m /= 10;
        x.e++;
    }
    x.m = (x.m + 5) / 10;
    x.e++;
    return x;
}

Decimal::Number Decimal::roundTowardsZero(Number x, int N) PRIME_NOEXCEPT
{
    if (N < 0)
        N = 0;
    if (x.e >= -N)
        return x;
    if (x.e < -(N + 30)) {
        memset(&x, 0, sizeof(x));
        return x;
    }
    while (x.e < -(N + 1)) {
        x.m /= 10;
        x.e++;
    }
    x.m /= 10;
    x.e++;
    return x;
}

Decimal::Number Decimal::roundHalfToEven(Number x, int N) PRIME_NOEXCEPT
{
    if (N < 0)
        N = 0;
    if (x.e >= -N)
        return x;
    if (x.e < -(N + 30)) {
        memset(&x, 0, sizeof(x));
        return x;
    }
    while (x.e < -(N + 1)) {
        x.m /= 10;
        x.e++;
    }
    if (x.m % 10 == 5) {
        x.m /= 10;
        if (x.m & 1) {
            ++x.m;
        }
        x.e++;

    } else {
        x.m = (x.m + 5) / 10;
        x.e++;
    }
    return x;
}

Decimal::Number Decimal::parse(const char* in, const char* end, bool allowExponent) PRIME_NOEXCEPT
{
    static const Number error_value = { 0, 0, maxExponent + 1, 0 };

    static const int64_t L10 = (INT64_MAX / 10);
    int aMaxFinal[2] = { 7, 8 };
    int bRnd = 1; // If mantissa overflows, round it
    int seenRadix = 0; // True after decimal point has been parsed
    int seenDigit = 0; // True after first non-zero digit parsed
    Number r; // Value to return
    char c;

    PRIME_COMPILE_TIME_ASSERT(INT64_MAX / 10 == 922337203685477580);

    memset(&r, 0, sizeof(r));

    /* Check for a leading '+' or '-' symbol. */
    if (in != end) {
        if (*in == '-') {
            r.sign = 1;
            ++in;
        } else if (*in == '+') {
            ++in;
        }
    }

    if (in == end) {
        return error_value;
    }

    /* Check for the string "inf". This is a special case. */
    if ((end - in) >= 3 && ASCIICompareIgnoringCase(in, "inf", 3) == 0) {
        r.e = maxExponent + 1;
        r.m = 1;
        in += 3;
        goto finished;
    }

    for (; in != end; ++in) {
        c = *in;
        if (c >= '0' && c <= '9') {
            int iDigit = (c - '0');

            if (iDigit == 0 && seenDigit == 0) {
                /* Handle leading zeroes. If they occur to the right of the decimal
                    ** point they can just be ignored. Otherwise, decrease the exponent
                    ** by one.  */
                if (seenRadix)
                    r.e--;
                continue;
            }

            seenDigit = 1;
            if (r.e > 0 || r.m > L10 || (r.m == L10 && iDigit > aMaxFinal[r.sign])) {
                /* Mantissa overflow. */
                if (seenRadix == 0)
                    r.e++;
                if (iDigit != 0) {
                    r.approx = 1;
                }
                if (bRnd) {
                    if (iDigit > 5 && r.m < ((uint64_t)INT64_MAX + r.sign))
                        r.m++;
                    bRnd = 0;
                }
            } else {
                if (seenRadix)
                    r.e -= 1;
                r.m = (r.m * 10) + (uint64_t)iDigit;
            }

        } else {
            if (c == '.') {
                /* Permit only a single radix in each number */
                if (seenRadix)
                    goto finished;
                seenRadix = 1;
            } else if (allowExponent && (c == 'e' || c == 'E')) {
                Number exp;
                if (in == end)
                    goto finished;
                ++in;
                exp = parse(in, end, false);
                if (isNaN(exp))
                    goto finished;
                if (exp.e || exp.m > 999)
                    goto finished;
                r.e += (short)((int)(exp.m) * (exp.sign ? -1 : 1));
                in = end;
                break;
            } else {
                goto finished;
            }
        }
    }

finished:

    if (in != end) {
        r.e = maxExponent + 1;
        r.m = 0;
    }

    return r;
}

Decimal::Number Decimal::fromInt64(int64_t from) PRIME_NOEXCEPT
{
    Number r;
    r.approx = 0;
    r.e = 0;
    r.sign = from < 0;
    if (from >= 0) {
        r.m = (uint64_t)from;
    } else if (from != INT64_MIN) {
        r.m = (uint64_t)(-from);
    } else {
        r.m = 1 + (uint64_t)INT64_MAX;
    }
    return r;
}

Decimal::Number Decimal::fromDouble(double from) PRIME_NOEXCEPT
{
    const double large = (double)UINT64_MAX;
    const double large10 = (double)oneTenthOfMax;
    Number x = { 0, 0, 0, 0 };

    /* TODO: How should this be set? */
    x.approx = 1;

    if (from < 0.0) {
        x.sign = 1;
        from = from * -1.0;
    }

    while (from > large || (from > 1.0 && from == (int64_t)from)) {
        from = from / 10.0;
        x.e++;
    }

    while (from < large10 && from != (double)((int64_t)from)) {
        from = from * 10.0;
        x.e--;
    }
    x.m = (uint64_t)from;

    return x;
}

int32_t Decimal::toInt32(Number number, bool& lossy) PRIME_NOEXCEPT
{
    int64_t iVal;
    iVal = toInt64(number, lossy);
    if (iVal < INT32_MIN) {
        lossy = true;
        return INT32_MIN;
    } else if (iVal > INT32_MAX) {
        lossy = true;
        return INT32_MAX;
    } else {
        return (int)iVal;
    }
}

bool Decimal::toDouble(Number number, double& output) PRIME_NOEXCEPT
{
    int i;
    double result = (double)(int64_t)number.m;
    if (number.sign)
        result = result * -1;
    for (i = 0; i < number.e; i++) {
        result = result * 10.0;
    }
    for (i = number.e; i < 0; i++) {
        result = result / 10.0;
    }
    output = result;
    return true;
}

int64_t Decimal::toInt64(Number number, bool& lossy) PRIME_NOEXCEPT
{
    static const int64_t L10 = (INT64_MAX / 10);
    uint64_t iRet;
    int i;
    iRet = number.m;

    lossy = number.approx != 0;
    for (i = 0; i < number.e; i++) {
        if (iRet > L10)
            goto overflow;
        iRet = iRet * 10;
    }
    for (i = number.e; i < 0; i++) {
        if (iRet % 10)
            lossy = true;
        iRet = iRet / 10;
    }

    if (number.sign) {
        if (iRet > (uint64_t)INT64_MAX + 1)
            goto overflow;
        return -(int64_t)iRet;
    } else {
        if (iRet > (uint64_t)INT64_MAX)
            goto overflow;
        return (int64_t)iRet;
    }

overflow:
    lossy = true;
    return number.sign ? -INT64_MAX - 1 : INT64_MAX;
}

static char* renderInt(uint64_t v, char* zBuf, int nBuf) PRIME_NOEXCEPT
{
    int i = nBuf - 1;
    zBuf[i--] = 0;
    do {
        zBuf[i--] = (v % 10) + '0';
        v /= 10;
    } while (v > 0);
    return zBuf + (i + 1);
}

static void removeTrailingZeros(char* z, int* pN) PRIME_NOEXCEPT
{
    int i = *pN;
    while (i > 0 && z[i - 1] == '0') {
        i--;
    }
    z[i] = 0;
    *pN = i;
}

int Decimal::toString(Number x, char* zOut, bool alwaysAppendFraction) PRIME_NOEXCEPT
{
    char zBuf[24];
    char* zNum;
    int n;
    static const char zeros[] = "0000000000000000000000000";

    char* z = zOut;

    if (x.sign && x.m > 0) {
        /* Add initial "-" for negative non-zero values */
        z[0] = '-';
        z++;
    }
    if (x.e > maxExponent) {
        /* Handle NaN and infinite values */
        if (x.m == 0) {
            memcpy(z, "NaN", 4);
        } else {
            memcpy(z, "inf", 4);
        }
        return (int)((z - zOut) + 3);
    }
    if (x.m == 0) {
        if (alwaysAppendFraction) {
            memcpy(z, "0.0", 4);
        } else {
            memcpy(z, "0", 2);
        }
        return (int)(1 + (z - zOut));
    }
    zNum = renderInt(x.m, zBuf, sizeof(zBuf));
    n = (int)(&zBuf[sizeof(zBuf) - 1] - zNum);
    if (x.e >= 0 && x.e + n <= 25) {
        /* Integer values with up to 25 digits */
        memcpy(z, zNum, (size_t)(n + 1));
        z += n;
        if (x.e > 0) {
            memcpy(z, zeros, (size_t)x.e);
            z += x.e;
            z[0] = 0;
        }
        if (alwaysAppendFraction) {
            memcpy(z, ".0", 3);
            z += 2;
        }
        return (int)(z - zOut);
    }
    if (x.e < 0 && n + x.e > 0) {
        /* Fractional values where the decimal point occurs within the
            ** significant digits.  ex:  12.345 */
        int m = n + x.e;
        memcpy(z, zNum, (size_t)m);
        z += m;
        zNum += m;
        n -= m;
        removeTrailingZeros(zNum, &n);
        if (n > 0) {
            z[0] = '.';
            z++;
            memcpy(z, zNum, (size_t)n);
            z += n;
            z[0] = 0;
        } else {
            if (alwaysAppendFraction) {
                memcpy(z, ".0", 3);
                z += 2;
            } else {
                z[0] = 0;
            }
        }
        return (int)(z - zOut);
    }
    if (x.e < 0 && x.e >= -n - 5) {
        /* Values less than 1 and with no more than 5 subsequent zeros prior
            ** to the first significant digit.  Ex:  0.0000012345 */
        int j = -(n + x.e);
        memcpy(z, "0.", 2);
        z += 2;
        if (j > 0) {
            memcpy(z, zeros, (size_t)j);
            z += j;
        }
        removeTrailingZeros(zNum, &n);
        memcpy(z, zNum, (size_t)n);
        z += n;
        z[0] = 0;
        return (int)(z - zOut);
    }
    /* Exponential notation from here to the end.  ex:  1.234e-15 */
    z[0] = zNum[0];
    z++;
    if (n > 1) {
        int nOrig = n;
        removeTrailingZeros(zNum, &n);
        x.e += (short)(nOrig - n);
    }
    if (n != 1) {
        /* Two or or more significant digits.  ex: 1.23e17 */
        *z++ = '.';
        memcpy(z, zNum + 1, (size_t)(n - 1));
        z += n - 1;
        x.e += (short)(n - 1);
    }
    *z++ = 'e';
    if (x.e < 0) {
        *z++ = '-';
        x.e = -x.e;
    } else {
        *z++ = '+';
    }
    zNum = renderInt(x.e & 0x7fff, zBuf, sizeof(zBuf));
    while ((z[0] = zNum[0]) != 0) {
        z++;
        zNum++;
    }
    return (int)(z - zOut);
}

//
// Decimal
//

Optional<Decimal> Decimal::fromString(StringView string) PRIME_NOEXCEPT
{
    Decimal d;
    if (!parse(d, string)) {
        return nullopt;
    }

    return d;
}

bool Decimal::parse(Decimal& decimal, StringView string) PRIME_NOEXCEPT
{
    decimal = parse(string.begin(), string.end(), true);

    return !decimal.isNaN();
}

Decimal Decimal::parseOr(StringView string, const Decimal& orElse) PRIME_NOEXCEPT
{
    Decimal result(parse(string.begin(), string.end(), true));

    if (result.isNaN()) {
        result = orElse;
    }

    return result;
}

Decimal::Decimal(double from) PRIME_NOEXCEPT : _num(fromDouble(from))
{
}

Decimal::Decimal(int from) PRIME_NOEXCEPT : _num(fromInt64(from))
{
}

Decimal::Decimal(int64_t from) PRIME_NOEXCEPT : _num(fromInt64(from))
{
}

int Decimal::toInt() const PRIME_NOEXCEPT
{
    int32_t out;
    return toInt(out) ? out : 0;
}

bool Decimal::toInt(int& out) const PRIME_NOEXCEPT
{
    return toInt32(out);
}

int32_t Decimal::toInt32() const PRIME_NOEXCEPT
{
    int32_t out;
    return toInt32(out) ? out : 0;
}

bool Decimal::toInt32(int32_t& out) const PRIME_NOEXCEPT
{
    bool lossy = true;
    out = toInt32(_num, lossy);
    return !lossy;
}

int64_t Decimal::toInt64() const PRIME_NOEXCEPT
{
    int64_t out;
    return toInt64(out) ? out : 0;
}

bool Decimal::toInt64(int64_t& out) const PRIME_NOEXCEPT
{
    bool lossy = true;
    out = toInt64(_num, lossy);
    return !lossy;
}

double Decimal::toDouble() const PRIME_NOEXCEPT
{
    double out;
    return toDouble(out) ? out : 0;
}

bool Decimal::toDouble(double& out) const PRIME_NOEXCEPT
{
    return toDouble(_num, out);
}

Decimal Decimal::getRounded(int digits, RoundMode roundMode) const PRIME_NOEXCEPT
{
    switch (roundMode) {
    default:
        PRIME_ASSERT(0);

    case RoundModeTowardsZero:
        return roundTowardsZero(_num, digits);

    case RoundModeHalfAwayFromZero:
        return round(_num, digits);

    case RoundModeHalfToEven:
        return roundHalfToEven(_num, digits);
    }
}

bool Decimal::isInfinite() const PRIME_NOEXCEPT
{
    return isInfinite(_num);
}

bool Decimal::isNaN() const PRIME_NOEXCEPT
{
    return isNaN(_num);
}

int Decimal::compare(const Decimal& rhs) const PRIME_NOEXCEPT
{
    switch (compare(_num, rhs._num)) {
    case CompareLess:
        return -1;
    case CompareEqual:
        return 0;
    case CompareGreater:
        return 1;
    default:
        return memcmp(&_num, &rhs._num, sizeof(_num));
    }
}

Decimal Decimal::operator+(const Decimal& rhs) const PRIME_NOEXCEPT
{
    return add(_num, rhs._num);
}

Decimal Decimal::operator-(const Decimal& rhs) const PRIME_NOEXCEPT
{
    return subtract(_num, rhs._num);
}

Decimal Decimal::operator*(const Decimal& rhs) const PRIME_NOEXCEPT
{
    return multiply(_num, rhs._num);
}

Decimal Decimal::operator/(const Decimal& rhs) const PRIME_NOEXCEPT
{
    return divide(_num, rhs._num);
}

Decimal& Decimal::operator+=(const Decimal& rhs) PRIME_NOEXCEPT
{
    _num = add(_num, rhs._num);
    return *this;
}

Decimal& Decimal::operator-=(const Decimal& rhs) PRIME_NOEXCEPT
{
    _num = subtract(_num, rhs._num);
    return *this;
}

Decimal& Decimal::operator*=(const Decimal& rhs) PRIME_NOEXCEPT
{
    _num = multiply(_num, rhs._num);
    return *this;
}

Decimal& Decimal::operator/=(const Decimal& rhs) PRIME_NOEXCEPT
{
    _num = divide(_num, rhs._num);
    return *this;
}

std::string Decimal::toString() const
{
    char buf[64];
    if (!toString(buf, sizeof(buf))) {
        buf[0] = '\0';
    }
    return buf;
}

std::string Decimal::toString(int digits, RoundMode roundMode) const
{
    char buf[64];
    if (!toString(buf, sizeof(buf), digits, roundMode)) {
        buf[0] = '\0';
    }
    return buf;
}

std::string Decimal::toStringWithThousandSeparator(char separator, int digits, RoundMode roundMode) const
{
    char buf[90];
    if (!toStringWithThousandSeparator(buf, sizeof(buf), separator, digits, roundMode)) {
        buf[0] = '\0';
    }
    return buf;
}

std::string Decimal::toStringWithThousandSeparator(char separator) const
{
    char buf[90];
    if (!toStringWithThousandSeparator(buf, sizeof(buf), separator)) {
        buf[0] = '\0';
    }
    return buf;
}

bool Decimal::toString(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT
{
    char buf[64];
    toString(_num, buf, 0);
    return StringCopy(buffer, bufferSize, buf);
}

bool Decimal::toString(char* buffer, size_t bufferSize, int digits, RoundMode roundMode) const PRIME_NOEXCEPT
{
    char buf[64];
    toString(getRounded(digits, roundMode)._num, buf, digits != 0);
    if (const char* dot = strchr(buf, '.')) {
        const char* end = buf + strlen(buf);
        int appendZeros = digits - (int)(end - dot) + 1;
        while (appendZeros-- > 0) {
            StringAppend(buf, sizeof(buf), "0");
        }
    }
    return StringCopy(buffer, bufferSize, buf);
}

bool Decimal::toStringWithThousandSeparator(char* buffer, size_t bufferSize, char separator, int digits,
    RoundMode roundMode) const PRIME_NOEXCEPT
{
    if (!toString(buffer, bufferSize, digits, roundMode)) {
        return false;
    }

    return insertThousandSeparators(buffer, bufferSize, separator);
}

bool Decimal::toStringWithThousandSeparator(char* buffer, size_t bufferSize, char separator) const PRIME_NOEXCEPT
{
    if (!toString(buffer, bufferSize)) {
        return false;
    }

    return insertThousandSeparators(buffer, bufferSize, separator);
}

bool Decimal::insertThousandSeparators(char* buffer, size_t bufferSize, char separator)
{
    size_t len = strlen(buffer);
    char* end = (char*)(buffer + len);
    if (char* dot = (char*)strchr(buffer, '.')) {
        end = dot;
    }

    char* ptr = end;
    for (;;) {
        if (ptr - buffer > 3 && ASCIIIsDigit(ptr[-4])) {
            ptr -= 3;
            if (len == bufferSize - 1) {
                return false;
            }
            memmove(&ptr[1], &ptr[0], len - (ptr - buffer) + 1);
            *ptr = separator;
            ++len;
        } else {
            break;
        }
    }

    return true;
}

//
// Decimal StringAppend support
//

bool StringAppend(std::string& string, const Decimal& decimal)
{
    string += decimal.toString();
    return true;
}

std::string ToStringEmptyIfZero(const Decimal& decimal)
{
    std::string result;

    if (!decimal.isZero()) {
        StringAppend(result, decimal);
    }

    return result;
}

bool UnsafeConvert(Decimal& decimal, StringView input) PRIME_NOEXCEPT
{
    return Decimal::parse(decimal, input);
}

bool UnsafeConvert(Value& output, const Decimal& decimal)
{
    return StringAppend(output.resetString(), decimal);
}

bool UnsafeConvert(Decimal& decimal, const Value& value)
{
    return UnsafeConvert(decimal, ToString(value));
}
}
