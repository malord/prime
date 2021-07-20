// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_COLOURS_H
#define PRIME_COLOURS_H

#include "NumberUtils.h"

namespace Prime {

//
// sRGB
//

template <typename Float>
inline Float SRGBToLinear(Float s)
{
    const Float a = Float(0.055);

    if (s <= Float(0.04045)) {
        return s / Float(12.92);
    }

    return Pow((s + a) / (Float(1) + a), Float(2.4));
}

template <typename Float>
inline Float LinearToSRGB(Float linear)
{
    const Float a = Float(0.055);

    if (linear <= Float(0.0031308)) {
        return Float(12.92) * linear;
    }

    return (Float(1) + a) * Pow(linear, Float(1) / Float(2.4)) - a;
}

//
// CMYK colour space
//

template <typename Float>
void CMYKToRGB(Float c, Float m, Float y, Float k, Float& r, Float& g, Float& b) PRIME_NOEXCEPT
{
    const Float one = Float(1);
    const Float oneMinusK = one - k;
    r = (one - c) * oneMinusK;
    g = (one - m) * oneMinusK;
    b = (one - y) * oneMinusK;
}

template <typename Float>
void CMYKToRGB(Float c, Float m, Float y, Float k, uint8_t& r, uint8_t& g, uint8_t& b) PRIME_NOEXCEPT
{
    const Float one = Float(1);
    const Float oneMinusK = Float(255) * (one - k);
    r = (uint8_t)((one - c) * oneMinusK);
    g = (uint8_t)((one - m) * oneMinusK);
    b = (uint8_t)((one - y) * oneMinusK);
}

inline void CMYKToRGB(uint8_t c, uint8_t m, uint8_t y, uint8_t k, uint8_t& r, uint8_t& g, uint8_t& b) PRIME_NOEXCEPT
{
    const unsigned int oneMinusK = 255u - k;
    r = (uint8_t)(((255u - c) * oneMinusK) / 255u);
    g = (uint8_t)(((255u - m) * oneMinusK) / 255u);
    b = (uint8_t)(((255u - y) * oneMinusK) / 255u);
}

//
// HSV colour space
//

/// Convet an H,S,V colour to the R,G,B colour space. h (hue) should be in the range 0 to 360, s and v should be
/// in the range 0..1. The outputs are in the range 0..1.
template <typename Float>
void HSVToRGB(Float h, Float s, Float v, Float& r, Float& g, Float& b) PRIME_NOEXCEPT
{
    int segment;
    Float f, q, p, t;

    if (s < Float(0.001)) {
        // Grey
        r = g = b = v;
        return;
    }

    h /= 60;
    segment = (int)floor(h);
    f = (Float)(h - segment);
    p = v * (Float(1.0) - s);
    q = v * (Float(1.0) - s * f);
    t = v * (Float(1.0) - s * (Float(1.0) - f));

    switch (segment) {
    case 0:
        r = v;
        g = t;
        b = p;
        break;

    case 1:
        r = q;
        g = v;
        b = p;
        break;

    case 2:
        r = p;
        g = v;
        b = t;
        break;

    case 3:
        r = p;
        g = q;
        b = v;
        break;

    case 4:
        r = t;
        g = p;
        b = v;
        break;

    default:
        r = v;
        g = p;
        b = q;
        break;
    }
}

/// Convet an H,S,V colour to the R,G,B colour space. h (hue) should be in the range 0 to 360, s and v should be
/// in the range 0..1. The outputs are in the range 0..255.
template <typename Float>
inline void HSVToRGB8(Float h, Float s, Float v, uint8_t& r, uint8_t& g, uint8_t& b) PRIME_NOEXCEPT
{
    Float fr, fg, fb;
    HSVToRGB(h, s, v, fr, fg, fb);
    r = (uint8_t)PRIME_CLAMP(fr * Float(255.0), Float(0), Float(255.0));
    g = (uint8_t)PRIME_CLAMP(fg * Float(255.0), Float(0), Float(255.0));
    b = (uint8_t)PRIME_CLAMP(fb * Float(255.0), Float(0), Float(255.0));
}

//
// YUV colour space
//

/// Compute the luminance (Y) of an R,G,B colour.
inline uint8_t RGBToLuminance8(uint32_t r, uint32_t g, uint32_t b) PRIME_NOEXCEPT
{
    return (uint8_t)((76 * r + 150 * g + 30 * b) / 256);
}

/// Compute the luminance (Y) of an R,G,B colour.
inline uint16_t RGBToLuminance16(uint32_t r, uint32_t g, uint32_t b) PRIME_NOEXCEPT
{
    return (uint16_t)((19595 * r + 38469 * g + 7472 * b) / 65536);
}

/// Compute the luminance (Y) of an R,G,B colour.
template <typename Float>
inline Float RGBToLuminance(Float r, Float g, Float b) PRIME_NOEXCEPT
{
    return Float(0.299) * r + Float(0.587) * g + Float(0.114) * b;
}

/// Convert an R,G,B colour to the Y,U,V colour space.
template <typename Float>
inline void RGBToYUV(Float r, Float g, Float b, Float& y, Float& u, Float& v) PRIME_NOEXCEPT
{
    y = Float(0.299) * r + Float(0.587) * g + Float(0.114) * b;
    u = Float(-0.147) * r - Float(0.289) * g + Float(0.436) * b;
    v = Float(0.615) * r - Float(0.515) * g - Float(0.100) * b;
}

/// Convert a Y,U,V colour to the R,G,B colour space.
template <typename Float>
inline void YUVToRGB(Float y, Float u, Float v, Float& r, Float& g, Float& b) PRIME_NOEXCEPT
{
    r = y + Float(1.13983) * v;
    g = y - Float(0.39456) * u - Float(0.58060) * v;
    b = y + Float(2.03211) * u;
}

/// Convert an R,G,B colour to the Y,U,V colour space.
template <typename Float>
inline void RGB8ToYUV8(uint8_t r, uint8_t g, uint8_t b, uint8_t* y, uint8_t* u, uint8_t* v) PRIME_NOEXCEPT
{
    Float fy, fu, fv;
    RGBToYUV(r, g, b, &fy, &fu, &fv);
    y = (uint8_t)PRIME_CLAMP(fy, Float(0.0), Float(255.0));
    u = (uint8_t)PRIME_CLAMP(fu, Float(0.0), Float(255.0));
    v = (uint8_t)PRIME_CLAMP(fv, Float(0.0), Float(255.0));
}

/// Convert a Y,U,V colour to the R,G,B colour space.
template <typename Float>
inline void YUV8ToRGB8(uint8_t y, uint8_t u, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b) PRIME_NOEXCEPT
{
    Float fr, fg, fb;
    YUVToRGB(y, u, v, &fr, &fg, &fb);
    r = (uint8_t)PRIME_CLAMP(fr, Float(0.0), Float(255.0));
    g = (uint8_t)PRIME_CLAMP(fg, Float(0.0), Float(255.0));
    b = (uint8_t)PRIME_CLAMP(fb, Float(0.0), Float(255.0));
}

/// Convert an R,G,B colour (0-0xffff per component) to the Y,U,V space.
template <typename Float>
inline void RGB16ToYUV16(unsigned int r, unsigned int g, unsigned int b, unsigned int& y, unsigned int& u, unsigned int& v) PRIME_NOEXCEPT
{
    Float fy, fu, fv;
    RGBToYUV((Float)r, (Float)g, (Float)b, &fy, &fu, &fv);
    y = (unsigned int)PRIME_CLAMP(fy, Float(0.0), Float(65535.0));
    u = (unsigned int)PRIME_CLAMP(fu, Float(0.0), Float(65535.0));
    v = (unsigned int)PRIME_CLAMP(fv, Float(0.0), Float(65535.0));
}

/// Convert a Y,U,V colour (0-0xffff per component) to the R,G,B space.
template <typename Float>
inline void YUV16ToRGB16(unsigned int y, unsigned int u, unsigned int v, unsigned int& r, unsigned int& g, unsigned int& b) PRIME_NOEXCEPT
{
    Float fr, fg, fb;
    YUVToRGB((Float)y, (Float)u, (Float)v, &fr, &fg, &fb);
    r = (unsigned int)PRIME_CLAMP(fr, Float(0.0), Float(65535.0));
    g = (unsigned int)PRIME_CLAMP(fg, Float(0.0), Float(65535.0));
    b = (unsigned int)PRIME_CLAMP(fb, Float(0.0), Float(65535.0));
}

//
// RGBA32 encoding
//

#ifdef PRIME_LITTLE_ENDIAN

#define PRIME_RGBA32(r, g, b, a) PRIME_MAKE32_BYTES(r, g, b, a)
#define PRIME_RGBA32_R(rgba32) PRIME_BYTE32(rgba32, 0)
#define PRIME_RGBA32_G(rgba32) PRIME_BYTE32(rgba32, 1)
#define PRIME_RGBA32_B(rgba32) PRIME_BYTE32(rgba32, 2)
#define PRIME_RGBA32_A(rgba32) PRIME_BYTE32(rgba32, 3)

#else

#define PRIME_RGBA32(r, g, b, a) PRIME_MAKE32_BYTES(a, b, g, r)
#define PRIME_RGBA32_R(rgba32) PRIME_BYTE32(rgba32, 3)
#define PRIME_RGBA32_G(rgba32) PRIME_BYTE32(rgba32, 2)
#define PRIME_RGBA32_B(rgba32) PRIME_BYTE32(rgba32, 1)
#define PRIME_RGBA32_A(rgba32) PRIME_BYTE32(rgba32, 0)

#endif

#define PRIME_RGBA32_FROM_FLOATS(fr, fg, fb, fa)                     \
    PRIME_RGBA32(                                                    \
        (unsigned int)PRIME_CLAMP((float)(fr)*255.0f, 0.0f, 255.0f), \
        (unsigned int)PRIME_CLAMP((float)(fg)*255.0f, 0.0f, 255.0f), \
        (unsigned int)PRIME_CLAMP((float)(fb)*255.0f, 0.0f, 255.0f), \
        (unsigned int)PRIME_CLAMP((float)(fa)*255.0f, 0.0f, 255.0f))

#define PRIME_RGBA32_FROM_FLOAT_ARRAY(arr) \
    PRIME_RGBA32_FROM_FLOATS((arr)[0], (arr)[1], (arr)[2], (arr)[3])

#define PRIME_RGBA32_FROM_ARRAY(arr) \
    PRIME_RGBA32((arr)[0], (arr)[1], (arr)[2], (arr)[3])

/// A 32-bit R,G,B,A colour stored in memory in that order.
class RGBA32 {
private:
    union {
        struct {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        } part;
        uint32_t whole;
        uint8_t array[4];
    } u;

public:
    /// A U32 colour is stored as bytes in the order R,G,B,A.
    static RGBA32 fromU32(uint32_t whole) PRIME_NOEXCEPT { return RGBA32(whole); }

    static RGBA32 fromFloats(float r, float g, float b, float a) PRIME_NOEXCEPT
    {
        return RGBA32((uint8_t)(r * 255.0f),
            (uint8_t)(g * 255.0f),
            (uint8_t)(b * 255.0f),
            (uint8_t)(a * 255.0f));
    }

    static RGBA32 fromArray(const float* array) PRIME_NOEXCEPT
    {
        return RGBA32((uint8_t)(array[0] * 255.0f),
            (uint8_t)(array[1] * 255.0f),
            (uint8_t)(array[2] * 255.0f),
            (uint8_t)(array[3] * 255.0f));
    }

    RGBA32()
    PRIME_NOEXCEPT { }

    RGBA32(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff)
    PRIME_NOEXCEPT
    {
        u.part.r = r;
        u.part.g = g;
        u.part.b = b;
        u.part.a = a;
    }

    explicit RGBA32(uint32_t whole) PRIME_NOEXCEPT { u.whole = whole; }

    uint8_t getR() const PRIME_NOEXCEPT { return u.part.r; }
    uint8_t getG() const PRIME_NOEXCEPT { return u.part.g; }
    uint8_t getB() const PRIME_NOEXCEPT { return u.part.b; }
    uint8_t getA() const PRIME_NOEXCEPT { return u.part.a; }

    float getRAsFloat() const PRIME_NOEXCEPT { return (float)u.part.r / 255.0f; }
    float getGAsFloat() const PRIME_NOEXCEPT { return (float)u.part.g / 255.0f; }
    float getBAsFloat() const PRIME_NOEXCEPT { return (float)u.part.b / 255.0f; }
    float getAAsFloat() const PRIME_NOEXCEPT { return (float)u.part.a / 255.0f; }

    void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a) PRIME_NOEXCEPT
    {
        u.part.r = r;
        u.part.g = g;
        u.part.b = b;
        u.part.a = a;
    }

    void setR(uint8_t value) PRIME_NOEXCEPT { u.part.r = value; }
    void setG(uint8_t value) PRIME_NOEXCEPT { u.part.g = value; }
    void setB(uint8_t value) PRIME_NOEXCEPT { u.part.b = value; }
    void setA(uint8_t value) PRIME_NOEXCEPT { u.part.a = value; }

    void setRFloat(float value) PRIME_NOEXCEPT { u.part.r = (uint8_t)(value * 255.0f); }
    void setGFloat(float value) PRIME_NOEXCEPT { u.part.g = (uint8_t)(value * 255.0f); }
    void setBFloat(float value) PRIME_NOEXCEPT { u.part.b = (uint8_t)(value * 255.0f); }
    void setAFloat(float value) PRIME_NOEXCEPT { u.part.a = (uint8_t)(value * 255.0f); }

    void setRGB(uint8_t grey) PRIME_NOEXCEPT { u.part.r = u.part.g = u.part.b = grey; }

    uint32_t toU32() const PRIME_NOEXCEPT { return u.whole; }

    template <typename Index>
    uint8_t operator[](Index index) const PRIME_NOEXCEPT { return u.array[index]; }

    template <typename Index>
    uint8_t& operator[](Index index) PRIME_NOEXCEPT { return u.array[index]; }

    bool operator==(const RGBA32& rhs) const PRIME_NOEXCEPT { return u.whole == rhs.u.whole; }
    bool operator<(const RGBA32& rhs) const PRIME_NOEXCEPT { return u.whole < rhs.u.whole; }
    PRIME_IMPLIED_COMPARISONS_OPERATORS(const RGBA32&)

    RGBA32 getAveraged(RGBA32 second) const PRIME_NOEXCEPT { return getAveraged(*this, second); }

    RGBA32 getScaled(RGBA32 second) const PRIME_NOEXCEPT { return getScaled(*this, second); }

    RGBA32 getAdded(RGBA32 second) const PRIME_NOEXCEPT { return getAdded(*this, second); }

    /// Returns colour1 * colour2 * 0.5, treating the uint32_t as a vector with 4 components.
    static RGBA32 getAveraged(RGBA32 colour1, RGBA32 colour2) PRIME_NOEXCEPT
    {
        RGBA32 result;

        for (int i = 0; i != 4; ++i) {
            uint32_t a = colour1.u.array[i];
            uint32_t b = colour2.u.array[i];
            result.u.array[i] = (uint8_t)((a + b) / 2);
        }

        return result;
    }

    /// Returns colour1 * colour2 / 255, treating the uint32_t as a vector with 4 components.
    static RGBA32 getScaled(RGBA32 colour1, RGBA32 colour2) PRIME_NOEXCEPT
    {
        RGBA32 result;

        for (int i = 0; i != 4; ++i) {
            uint32_t a = colour1.u.array[i];
            uint32_t b = colour2.u.array[i];
            result.u.array[i] = (uint8_t)((a * b) / 255);
        }

        return result;
    }

    /// Returns colour1 + colour2, with clamping.
    static RGBA32 getAdded(RGBA32 colour1, RGBA32 colour2) PRIME_NOEXCEPT
    {
        RGBA32 result;

        for (int i = 0; i != 4; ++i) {
            uint32_t a = colour1.u.array[i];
            uint32_t b = colour2.u.array[i];
            result.u.array[i] = (uint8_t)std::min<uint32_t>(a + b, 0xff);
        }

        return result;
    }
};
}

#endif
