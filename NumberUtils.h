// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_NUMBERS_H
#define PRIME_NUMBERS_H

#include "Config.h"
#include <math.h>

namespace Prime {

//
// Numbers
//

template <typename Type>
inline Type Clamp(const Type value, const Type low, const Type high) PRIME_NOEXCEPT
{
    return (value < low) ? low : (value > high) ? high : value;
}

template <typename Type>
inline Type Mix(const Type a, const Type b, const Type fraction) PRIME_NOEXCEPT
{
    return a * (Type(1) - fraction) + b * fraction;
}

template <typename Type>
inline Type Abs(Type value) PRIME_NOEXCEPT
{
    return value < 0 ? -value : value;
}

template <>
inline float Abs(float value) PRIME_NOEXCEPT
{
    return fabsf(value);
}

template <>
inline double Abs(double value) PRIME_NOEXCEPT
{
    return fabs(value);
}

inline float Pow(float x, float y)
{
    return powf(x, y);
}

inline double Pow(double x, double y)
{
    return pow(x, y);
}

inline long double Pow(long double x, long double y)
{
    return powl(x, y);
}

template <typename Type>
inline Type Mod(Type a, Type b) PRIME_NOEXCEPT
{
    return a % b;
}

template <>
inline float Mod(float a, float b) PRIME_NOEXCEPT
{
    return fmodf(a, b);
}

template <>
inline double Mod(double a, double b) PRIME_NOEXCEPT
{
    return fmod(a, b);
}

template <typename Type>
inline bool AlmostEqual(Type a, Type b, Type tolerance) PRIME_NOEXCEPT
{
    return Abs(a - b) <= tolerance;
}

template <typename Type>
inline Type Sign(Type value) PRIME_NOEXCEPT
{
    if (value < 0) {
        return (Type)-1;
    }

    if (value > 0) {
        return (Type)1;
    }

    return 0;
}

template <typename Type>
inline Type Sign(Type value, Type epsilon) PRIME_NOEXCEPT
{
    if (value < -epsilon) {
        return (Type)-1;
    }

    if (value > epsilon) {
        return (Type)1;
    }

    return 0;
}

/// std::min is occasionally missing or unavailable on older Windows compilers.
template <typename Type>
Type Min(const Type& a, const Type& b) PRIME_NOEXCEPT
{
    return a < b ? a : b;
}

/// std::max is occasionally missing or unavailable on older Windows compilers.
template <typename Type>
Type Max(const Type& a, const Type& b) PRIME_NOEXCEPT
{
    return a > b ? a : b;
}

// Narrow<T> moved to Common.h

//
// Integers
//

/// Returns the positive remainder of numerator / denominator.
template <typename Integer>
inline Integer PositiveModulus(Integer numerator, Integer denominator) PRIME_NOEXCEPT
{
    Integer result = numerator % denominator;
    return result < 0 ? denominator + result : result;
}

/// Given a number, "value", returns that number so it lies within the range [minimum, maximumPlusOne).
template <typename Integer>
inline Integer IntegerWrap(Integer value, Integer minimum, Integer maximumPlusOne) PRIME_NOEXCEPT
{
    return PositiveModulus<Integer>(value - minimum, maximumPlusOne - minimum) + minimum;
}

/// Computes the log base 2 of n, which must be of an integer type (unsigned or not negative).
template <typename Integer>
int IntLog2(Integer n) PRIME_NOEXCEPT
{
    int l = 0;

    while ((n >>= 1) != 0) {
        ++l;
    }

    return l;
}

/// Compute value raised to power.
template <typename Integer>
static Integer IntPow(Integer value, int power) PRIME_NOEXCEPT
{
    Integer result = 1;
    for (; power > 0; --power) {
        result *= value;
    }

    return result;
}

template <typename Integer>
inline bool IntIsPow2(Integer n) PRIME_NOEXCEPT
{
    return (n & (n - 1)) == 0;
}

/// Returns the next power of 2 greater than n (e.g., 16 -> 32, 17 -> 32).
inline uint64_t NextPow2(uint64_t n) PRIME_NOEXCEPT
{
    uint64_t m;

    m = n;
    m = (m >> 1) | m;
    m = (m >> 2) | m;
    m = (m >> 4) | m;
    m = (m >> 8) | m;
    m = (m >> 16) | m;
    m = (m >> 32) | m;

    return m + 1;
}

/// Returns the next power of 2 greater than n (e.g., 16 -> 32, 17 -> 32).
inline uint32_t NextPow2(uint32_t n) PRIME_NOEXCEPT
{
    uint32_t m;

    m = n;
    m = (m >> 1) | m;
    m = (m >> 2) | m;
    m = (m >> 4) | m;
    m = (m >> 8) | m;
    m = (m >> 16) | m;

    return m + 1;
}

/// Returns the next power of 2 greater than or equal to n (e.g., 16 -> 16, 17 -> 32).
inline uint64_t NearestPow2(uint64_t n) PRIME_NOEXCEPT
{
    return NextPow2(n - 1);
}

/// Returns the next power of 2 greater than or equal to n (e.g., 16 -> 16, 17 -> 32).
inline uint32_t NearestPow2(uint32_t n) PRIME_NOEXCEPT
{
    return NextPow2(n - 1);
}

/// Count the number of zero bits at the low end of any unsigned integer, which must not be zero.
template <typename UnsignedInteger>
int CountLowZeros(UnsignedInteger value) PRIME_NOEXCEPT
{
    PRIME_ASSERT(value != 0);

    // Could use __builtin_ctz / __builtin_ctzl or any of the bit-length specific algorithms (e.g. http://aggregate.org/MAGIC/#Trailing%20Zero%20Count)

    int count = 0;
    while ((value & UnsignedInteger(1)) == 0) {
        ++count;
        value >>= 1;
    }

    return count;
}

/// Count the leading zero bits at the high end of any unsigned integer, which must not be zero.
template <typename UnsignedInteger>
int CountHighZeros(UnsignedInteger value) PRIME_NOEXCEPT
{
    PRIME_ASSERT(value != 0);

    // Could use builtins or trick algorithm for this...

    const UnsignedInteger topBit = ~(UnsignedInteger(-1) >> 1);

    int count = 0;
    while (!(value & topBit)) {
        ++count;
        value <<= 1;
    }

    return count;
}

inline static uint32_t LeftRotate32(uint32_t n, unsigned int m) PRIME_NOEXCEPT
{
    return (n << m) | (n >> (32u - m));
}

#ifndef PRIME_NO_INT64

inline static uint64_t LeftRotate64(uint64_t n, unsigned int m) PRIME_NOEXCEPT
{
    return (n << m) | (n >> (64u - m));
}

#endif

inline static uint32_t RightRotate32(uint32_t n, unsigned int m) PRIME_NOEXCEPT
{
    return (n >> m) | (n << (32u - m));
}

#ifndef PRIME_NO_INT64

inline static uint64_t RightRotate64(uint64_t n, unsigned int m) PRIME_NOEXCEPT
{
    return (n >> m) | (n << (64u - m));
}

#endif

/// StaticIntLog<integer>::value is the log base 2 of integer, computed by the compiler.
template <intmax_t Integer>
struct StaticIntLog2 {
    PRIME_STATIC_CONST(intmax_t, value, 1 + StaticIntLog2<PRIME_MAX(Integer / 2, 0)>::value);
};

template <>
struct StaticIntLog2<1> {
    PRIME_STATIC_CONST(intmax_t, value, 0);
};

template <>
struct StaticIntLog2<0> {
    PRIME_STATIC_CONST(intmax_t, value, 0);
};

//
// Rounding
//

/// Round a positive integer up.
template <typename Integer>
inline Integer RoundUp(Integer value, Integer alignment) PRIME_NOEXCEPT
{
    return (value + (alignment - 1)) / alignment * alignment;
}

/// Round a positive integer down.
template <typename Integer>
inline Integer RoundDown(Integer value, Integer alignment) PRIME_NOEXCEPT
{
    return (value / alignment) * alignment;
}

/// Round a positive integer up.
template <typename Integer>
inline Integer RoundUpPow2(Integer value, Integer alignment) PRIME_NOEXCEPT
{
    PRIME_DEBUG_ASSERT(IntIsPow2<Integer>(alignment));
    return (value + (alignment - 1)) & (~(alignment - 1));
}

/// Round a positive integer down.
template <typename Integer>
inline Integer RoundDownPow2(Integer value, Integer alignment) PRIME_NOEXCEPT
{
    PRIME_DEBUG_ASSERT(IntIsPow2<Integer>(alignment));
    return value & (~(alignment - 1));
}

//
// Pointer arithmetic
//

template <typename Type>
inline Type* PointerAdd(Type* ptr, ptrdiff_t stride) PRIME_NOEXCEPT
{
    return reinterpret_cast<Type*>(reinterpret_cast<char*>(ptr) + stride);
}

template <typename Type>
inline Type* PointerSubtract(Type* ptr, ptrdiff_t stride) PRIME_NOEXCEPT
{
    return reinterpret_cast<Type*>(reinterpret_cast<char*>(ptr) - stride);
}

template <typename Type>
inline const Type* PointerAdd(const Type* ptr, ptrdiff_t stride) PRIME_NOEXCEPT
{
    return reinterpret_cast<const Type*>(reinterpret_cast<const char*>(ptr) + stride);
}

template <typename Type>
inline const Type* PointerSubtract(const Type* ptr, ptrdiff_t stride) PRIME_NOEXCEPT
{
    return reinterpret_cast<const Type*>(reinterpret_cast<const char*>(ptr) - stride);
}

template <typename Type1, typename Type2>
inline ptrdiff_t PointerDistance(const Type1* a, const Type2* b) PRIME_NOEXCEPT
{
    return reinterpret_cast<const char*>(a) - reinterpret_cast<const char*>(b);
}

/// Align a pointer up to a boundary.
inline void* AlignUp(void* ptr, size_t alignment) PRIME_NOEXCEPT
{
    return (void*)RoundUp<size_t>((size_t)ptr, alignment);
}

/// Align a pointer up to a boundary.
inline const void* AlignUp(const void* ptr, size_t alignment) PRIME_NOEXCEPT
{
    return (void*)RoundUp<size_t>((size_t)ptr, alignment);
}

/// Align a pointer up to a power-of-two boundary.
inline void* AlignUpPow2(void* ptr, size_t alignment) PRIME_NOEXCEPT
{
    return (void*)RoundUpPow2<size_t>((size_t)ptr, alignment);
}

/// Align a pointer up to a power-of-two boundary.
inline const void* AlignUpPow2(const void* ptr, size_t alignment) PRIME_NOEXCEPT
{
    return (void*)RoundUpPow2<size_t>((size_t)ptr, alignment);
}

/// Align a pointer down to a boundary.
inline void* AlignDown(void* ptr, size_t alignment) PRIME_NOEXCEPT
{
    return (void*)RoundDown<size_t>((size_t)ptr, alignment);
}

/// Align a pointer down to a boundary.
inline const void* AlignDown(const void* ptr, size_t alignment) PRIME_NOEXCEPT
{
    return (void*)RoundDown<size_t>((size_t)ptr, alignment);
}

/// Align a pointer down to a power-of-two boundary.
inline void* AlignDownPow2(void* ptr, size_t alignment) PRIME_NOEXCEPT
{
    return (void*)RoundDownPow2<size_t>((size_t)ptr, alignment);
}

/// Align a pointer down to a power-of-two boundary.
inline const void* AlignDownPow2(const void* ptr, size_t alignment) PRIME_NOEXCEPT
{
    return (void*)RoundDownPow2<size_t>((size_t)ptr, alignment);
}

//
// Trig
//

template <typename Float>
struct Trig {
    static const Float pi;
    static const Float twoPi;
    static const Float piOverTwo;
    static const Float piOverFour;
    static const Float oneOverPi;
    static const Float twoOverPi;
};

template <typename Float>
const Float Trig<Float>::pi = static_cast<Float>(PRIME_PI);

template <typename Float>
const Float Trig<Float>::twoPi = static_cast<Float>(PRIME_PI * 2.0);

template <typename Float>
const Float Trig<Float>::piOverTwo = static_cast<Float>(PRIME_PI / 2.0);

template <typename Float>
const Float Trig<Float>::piOverFour = static_cast<Float>(PRIME_PI / 4.0);

template <typename Float>
const Float Trig<Float>::oneOverPi = static_cast<Float>(1.0 / PRIME_PI);

template <typename Float>
const Float Trig<Float>::twoOverPi = static_cast<Float>(2.0 / PRIME_PI);

//
// Trigonometry
//

template <typename Float>
inline Float DegreesToRadians(Float degrees) PRIME_NOEXCEPT
{
    return degrees * Float(PRIME_PI / 180.0);
}

template <typename Float>
inline Float RadiansToDegrees(Float radians) PRIME_NOEXCEPT
{
    return radians * Float(180.0 / PRIME_PI);
}

template <typename Float>
inline Float WrapToTwoPi(Float radians) PRIME_NOEXCEPT
{
    return Mod(radians + Trig<Float>::twoPi, Trig<Float>::twoPi);
}

template <typename Float>
inline Float WrapToPi(Float radians) PRIME_NOEXCEPT
{
    return Mod(radians + Trig<Float>::pi, Trig<Float>::twoPi) - Trig<Float>::pi;
}

template <typename Float>
inline Float RadiansDifference(Float was, Float is) PRIME_NOEXCEPT
{
    Float n = WrapToPi(is) - WrapToPi(was);
    if (n > Trig<Float>::pi) {
        n -= Trig<Float>::twoPi;
    } else if (n < -Trig<Float>::pi) {
        n += Trig<Float>::twoPi;
    }
    return n;
}
}

#endif
