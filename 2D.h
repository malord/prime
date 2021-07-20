// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_2D_H
#define PRIME_2D_H

#include "Config.h"
#include <algorithm>

namespace Prime {

template <typename Type>
class Dimensions;

//
// Point
//

template <typename Type>
class Point {
public:
    Type x, y;

    typedef Prime::Dimensions<Type> Dimensions;

    static Point fromArray(const Type* array) PRIME_NOEXCEPT
    {
        return Point(array[0], array[1]);
    }

    template <typename Array>
    static Point fromArray(const Array& array) PRIME_NOEXCEPT
    {
        return Point(array[0], array[1]);
    }

    Point() PRIME_NOEXCEPT : x(0),
                             y(0)
    {
    }

    Point(Type initX, Type initY) PRIME_NOEXCEPT : x(initX),
                                                   y(initY)
    {
    }

    explicit Point(Type xAndY) PRIME_NOEXCEPT : x(xAndY),
                                                y(xAndY)
    {
    }

    Point(const Point& copy) PRIME_NOEXCEPT : x(copy.x),
                                              y(copy.y)
    {
    }

    void set(Type newX, Type newY) PRIME_NOEXCEPT
    {
        x = newX;
        y = newY;
    }

    Point& operator=(const Point& copy) PRIME_NOEXCEPT
    {
        x = copy.x;
        y = copy.y;
        return *this;
    }

    Dimensions toDimensions() const PRIME_NOEXCEPT;

    /// Returns a different kind of Point, e.g., Point<int>(3, 4).to<float>() returns a Point<float>.
    template <typename Other>
    Point<Other> to() const PRIME_NOEXCEPT
    {
        return Point<Other>(Other(x), Other(y));
    }

    void setX(Type value) PRIME_NOEXCEPT { x = value; }
    void setY(Type value) PRIME_NOEXCEPT { y = value; }

    Type getMaxCoord() const PRIME_NOEXCEPT { return std::max(x, y); }
    Type getMinCoord() const PRIME_NOEXCEPT { return std::min(x, y); }

    void swapXY() PRIME_NOEXCEPT { std::swap(x, y); }
    Point getSwappedXY() const PRIME_NOEXCEPT { return Point(y, x); }

    bool operator==(const Point& rhs) const PRIME_NOEXCEPT { return x == rhs.x && y == rhs.y; }
    bool operator<(const Point& rhs) const PRIME_NOEXCEPT { return x < rhs.x || (x == rhs.x && y < rhs.y); }
    PRIME_IMPLIED_COMPARISONS_OPERATORS(const Point&)

    Point& operator+=(const Point& rhs) PRIME_NOEXCEPT
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Point& operator-=(const Point& rhs) PRIME_NOEXCEPT
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    Point& operator*=(const Point& rhs) PRIME_NOEXCEPT
    {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    Point& operator/=(const Point& rhs) PRIME_NOEXCEPT
    {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

    Point operator-() const PRIME_NOEXCEPT { return Point(-x, -y); }
    Point operator+(const Point& rhs) const PRIME_NOEXCEPT { return Point(x + rhs.x, y + rhs.y); }
    Point operator-(const Point& rhs) const PRIME_NOEXCEPT { return Point(x - rhs.x, y - rhs.y); }
    Point operator*(const Point& rhs) const PRIME_NOEXCEPT { return Point(x * rhs.x, y * rhs.y); }
    Point operator/(const Point& rhs) const PRIME_NOEXCEPT { return Point(x / rhs.x, y / rhs.y); }

    Point getMins(const Point& other) const PRIME_NOEXCEPT { return getMins(other.x, other.y); }
    Point getMaxs(const Point& other) const PRIME_NOEXCEPT { return getMaxs(other.x, other.y); }

    Point getMins(Type otherX, Type otherY) const PRIME_NOEXCEPT
    {
        return Point(std::min(x, otherX), std::min(y, otherY));
    }

    Point getMaxs(Type otherX, Type otherY) const PRIME_NOEXCEPT
    {
        return Point(std::max(x, otherX), std::max(y, otherY));
    }
};

//
// Dimensions
//

template <typename Type>
class Dimensions {
public:
    Type width, height;

    typedef Prime::Point<Type> Point;

    static Dimensions fromArray(const Type* array) PRIME_NOEXCEPT
    {
        return Dimensions(array[0], array[1]);
    }

    template <typename Array>
    static Dimensions fromArray(const Array& array) PRIME_NOEXCEPT
    {
        return Dimensions(array[0], array[1]);
    }

    Dimensions() PRIME_NOEXCEPT : width(0),
                                  height(0)
    {
    }

    Dimensions(Type initWidth, Type initHeight) PRIME_NOEXCEPT : width(initWidth),
                                                                 height(initHeight)
    {
    }

    explicit Dimensions(Type widthAndHeight) PRIME_NOEXCEPT : width(widthAndHeight),
                                                              height(widthAndHeight)
    {
    }

    Dimensions(const Dimensions& copy) PRIME_NOEXCEPT : width(copy.width),
                                                        height(copy.height)
    {
    }

    void set(Type newWidth, Type newHeight) PRIME_NOEXCEPT
    {
        width = newWidth;
        height = newHeight;
    }

    Dimensions& operator=(const Dimensions& copy) PRIME_NOEXCEPT
    {
        width = copy.width;
        height = copy.height;
        return *this;
    }

    bool isZero() const PRIME_NOEXCEPT { return width <= 0 || height <= 0; }

    bool isZero(Type epsilon) const PRIME_NOEXCEPT { return width <= epsilon || height <= epsilon; }

    Point toPoint() const PRIME_NOEXCEPT { return Point(width, height); }

    Type getArea() const PRIME_NOEXCEPT { return width * height; }

    /// Returns a different kind of Dimensions, e.g., Dimensions<int>(3, 4).to<float>() returns a Dimensions<float>.
    template <typename Other>
    Dimensions<Other> to() const PRIME_NOEXCEPT
    {
        return Dimensions<Other>(Other(width), Other(height));
    }

    void setWidth(Type value) PRIME_NOEXCEPT { width = value; }
    void setHeight(Type value) PRIME_NOEXCEPT { height = value; }

    Type getMaxDimension() const PRIME_NOEXCEPT { return std::max(width, height); }
    Type getMinDimension() const PRIME_NOEXCEPT { return std::min(width, height); }

    void swapDimensions() PRIME_NOEXCEPT { std::swap(width, height); }
    Dimensions getSwappedDimensions() const PRIME_NOEXCEPT { return Dimensions(height, width); }

    bool operator==(const Dimensions& rhs) const PRIME_NOEXCEPT { return width == rhs.width && height == rhs.height; }
    bool operator<(const Dimensions& rhs) const PRIME_NOEXCEPT { return width < rhs.width || (width == rhs.width && height < rhs.height); }
    PRIME_IMPLIED_COMPARISONS_OPERATORS(const Dimensions&)

    Dimensions& operator+=(const Dimensions& rhs) PRIME_NOEXCEPT
    {
        width += rhs.width;
        height += rhs.height;
        return *this;
    }

    Dimensions& operator-=(const Dimensions& rhs) PRIME_NOEXCEPT
    {
        width -= rhs.width;
        height -= rhs.height;
        return *this;
    }

    Dimensions& operator*=(const Dimensions& rhs) PRIME_NOEXCEPT
    {
        width *= rhs.width;
        height *= rhs.height;
        return *this;
    }

    Dimensions& operator/=(const Dimensions& rhs) PRIME_NOEXCEPT
    {
        width /= rhs.width;
        height /= rhs.height;
        return *this;
    }

    Dimensions operator-() const { return Dimensions(-width, -height); }
    Dimensions operator+(const Dimensions& rhs) const PRIME_NOEXCEPT { return Dimensions(width + rhs.width, height + rhs.height); }
    Dimensions operator-(const Dimensions& rhs) const PRIME_NOEXCEPT { return Dimensions(width - rhs.width, height - rhs.height); }
    Dimensions operator*(const Dimensions& rhs) const PRIME_NOEXCEPT { return Dimensions(width * rhs.width, height * rhs.height); }
    Dimensions operator/(const Dimensions& rhs) const PRIME_NOEXCEPT { return Dimensions(width / rhs.width, height / rhs.height); }

    Dimensions getMins(const Dimensions& other) const PRIME_NOEXCEPT { return getMins(other.width, other.height); }
    Dimensions getMaxs(const Dimensions& other) const PRIME_NOEXCEPT { return getMaxs(other.width, other.height); }

    Dimensions getMins(Type otherWidth, Type otherHeight) const PRIME_NOEXCEPT
    {
        return Dimensions(std::min(width, otherWidth), std::min(height, otherHeight));
    }

    Dimensions getMaxs(Type otherWidth, Type otherHeight) const PRIME_NOEXCEPT
    {
        return Dimensions(std::max(width, otherWidth), std::max(height, otherHeight));
    }
};

//
// Rect
//

/// A 2D rectangle defined by its minimum and maximum coordinates along both axes.
template <typename Type>
class Rect {
public:
    typedef Prime::Point<Type> Point;
    typedef Prime::Dimensions<Type> Dimensions;

    Point mins;
    Point maxs;

    static Rect fromArray(const Type* array) PRIME_NOEXCEPT
    {
        return Rect(Point::fromArray(array), Dimensions::fromArray(array + 2));
    }

    template <typename Array>
    static Rect fromArray(const Array& array) PRIME_NOEXCEPT
    {
        return Rect(Point::fromArray(array), Dimensions::fromArray(&array[2]));
    }

    Rect() PRIME_NOEXCEPT { }

    Rect(Type minX, Type minY, Type maxX, Type maxY) PRIME_NOEXCEPT : mins(minX, minY), maxs(maxX, maxY)
    {
    }

    Rect(const Point& initMins, const Point& initMaxs) PRIME_NOEXCEPT : mins(initMins), maxs(initMaxs)
    {
    }

    Rect(const Point& initMins, const Dimensions& dimensions) PRIME_NOEXCEPT : mins(initMins), maxs(initMins + dimensions.toPoint())
    {
    }

    Rect(const Rect& copy) PRIME_NOEXCEPT : mins(copy.mins), maxs(copy.maxs)
    {
    }

    Rect& operator=(const Rect& copy) PRIME_NOEXCEPT
    {
        mins = copy.mins;
        maxs = copy.maxs;
        return *this;
    }

    bool operator==(const Rect& rhs) const PRIME_NOEXCEPT { return mins == rhs.mins && maxs == rhs.maxs; }
    bool operator<(const Rect& rhs) const PRIME_NOEXCEPT { return mins < rhs.mins || (mins == rhs.mins && maxs < rhs.maxs); }
    PRIME_IMPLIED_COMPARISONS_OPERATORS(const Rect&)

    void set(Type minX, Type minY, Type maxX, Type maxY) PRIME_NOEXCEPT
    {
        mins.set(minX, minY);
        maxs.set(maxX, maxY);
    }

    void set(const Point& newMins, const Point& newMaxs) PRIME_NOEXCEPT
    {
        mins = newMins;
        maxs = newMaxs;
    }

    void set(const Point& newMins, const Dimensions& dimensions) PRIME_NOEXCEPT
    {
        mins = newMins;
        maxs = newMins + dimensions.toPoint();
    }

    void setWidth(Type width) PRIME_NOEXCEPT { maxs.x = mins.x + width; }

    void setHeight(Type height) PRIME_NOEXCEPT { maxs.y = mins.y + height; }

    void setDimensions(Type width, Type height) PRIME_NOEXCEPT
    {
        setWidth(width);
        setHeight(height);
    }

    void setDimensions(const Dimensions& dimensions) PRIME_NOEXCEPT { maxs = mins + dimensions.toPoint(); }

    void clear()
    {
        mins.set(0, 0);
        maxs.set(0, 0);
    }

    bool isNormalised() const PRIME_NOEXCEPT { return maxs.x >= mins.x && maxs.y >= mins.y; }

    /// Ensure mins.x <= maxs.x and mins.y <= maxs.y.
    void normalise() PRIME_NOEXCEPT
    {
        if (mins.x > maxs.x) {
            std::swap(mins.x, maxs.x);
        }

        if (mins.y > maxs.y) {
            std::swap(mins.y, maxs.y);
        }
    }

    Rect getNormalised() const PRIME_NOEXCEPT
    {
        Rect<Type> rect(*this);
        rect.normalise();
        return rect;
    }

    Type getWidth() const PRIME_NOEXCEPT { return maxs.x - mins.x; }

    Type getHeight() const PRIME_NOEXCEPT { return maxs.y - mins.y; }

    Dimensions getDimensions() const PRIME_NOEXCEPT { return Dimensions(getWidth(), getHeight()); }

    Point getCentre() const PRIME_NOEXCEPT { return (mins + maxs) / Point(Type(2)); }

    /// Returns true if this rectangle is empty (0 or negative dimensions).
    bool isEmpty() const PRIME_NOEXCEPT { return maxs.x <= mins.x || maxs.y <= mins.y; }

    /// Returns true if this rectangle is empty.
    bool isEmpty(Type epsilon) const PRIME_NOEXCEPT { return maxs.x <= mins.x + epsilon || maxs.y <= mins.y + epsilon; }

    bool contains(const Point& point) const PRIME_NOEXCEPT
    {
        return contains(point.x, point.y);
    }

    bool contains(Type x, Type y) const PRIME_NOEXCEPT
    {
        return x >= mins.x && x < maxs.x && y >= mins.y && y < maxs.y;
    }

    bool encloses(const Rect& rect) const PRIME_NOEXCEPT
    {
        return rect.mins.x >= mins.x && rect.mins.y >= mins.y && rect.maxs.x <= maxs.x && rect.maxs.y <= maxs.y;
    }

    bool intersects(const Rect& rect) const PRIME_NOEXCEPT
    {
        return !(rect.maxs.x <= mins.x || rect.mins.x >= maxs.x || rect.maxs.y <= mins.y || rect.mins.y >= maxs.y);
    }

    Rect getInflated(Type dx, Type dy) const PRIME_NOEXCEPT
    {
        return Rect<Type>(mins.x - dx, mins.y - dy, maxs.x + dx, maxs.y + dy);
    }

    Rect getInflated(const Dimensions& dimensions) const PRIME_NOEXCEPT
    {
        return getInflated(dimensions.width, dimensions.height);
    }

    Rect getDeflated(Type dx, Type dy) const PRIME_NOEXCEPT
    {
        return getInflated(-dx, -dy);
    }

    Rect getDeflated(const Dimensions& dimensions) const PRIME_NOEXCEPT
    {
        return getDeflated(dimensions.width, dimensions.height);
    }

    void inflate(Type dx, Type dy) PRIME_NOEXCEPT
    {
        mins.x -= dx;
        mins.y -= dy;
        maxs.x += dx;
        maxs.y += dy;
    }

    void inflate(const Dimensions& dimensions) PRIME_NOEXCEPT
    {
        inflate(dimensions.width, dimensions.height);
    }

    void deflate(Type dx, Type dy) PRIME_NOEXCEPT
    {
        inflate(-dx, -dy);
    }

    void deflate(const Dimensions& dimensions) PRIME_NOEXCEPT
    {
        deflate(dimensions.width, dimensions.height);
    }

    void offset(const Point& distance) PRIME_NOEXCEPT
    {
        mins += distance;
        maxs += distance;
    }

    void offset(Type dx, Type dy) PRIME_NOEXCEPT
    {
        offset(Point(dx, dy));
    }

    Rect& operator+=(const Point& rhs) PRIME_NOEXCEPT
    {
        offset(rhs);
        return *this;
    }

    Rect& operator-=(const Point& rhs) PRIME_NOEXCEPT
    {
        offset(-rhs);
        return *this;
    }

    Rect operator+(const Point& rhs) const PRIME_NOEXCEPT { return Rect(mins + rhs, maxs + rhs); }

    Rect operator-(const Point& rhs) const PRIME_NOEXCEPT { return Rect(mins - rhs, maxs - rhs); }

    /// Subtract the specified rectangle from this rectangle. This rectangle and the source rectangle are
    /// unchanged, but up to 4 rectangles may be created (along each side). The rectsOut parameter must be an
    /// array with room for 4 rectangles. Some of the output rectangles may be empty or have negative dimensions
    /// if they were completely subtracted (check for those with isEmpty()).
    void subtract(const Rect& second, Rect rectsOut[4]) const PRIME_NOEXCEPT
    {
        rectsOut[0].mins.x = mins.x;
        rectsOut[0].maxs.x = maxs.x;
        rectsOut[0].mins.y = mins.y;
        rectsOut[0].maxs.y = std::min(second.mins.y, maxs.y);

        rectsOut[1].mins.x = mins.x;
        rectsOut[1].maxs.x = std::min(second.mins.x, maxs.x);
        rectsOut[1].mins.y = std::max(mins.y, second.mins.y);
        rectsOut[1].maxs.y = std::min(second.maxs.y, maxs.y);

        rectsOut[2].mins.x = std::max(mins.x, second.maxs.x);
        rectsOut[2].maxs.x = maxs.x;
        rectsOut[2].mins.y = std::max(mins.y, second.mins.y);
        rectsOut[2].maxs.y = std::min(second.maxs.y, maxs.y);

        rectsOut[3].mins.x = mins.x;
        rectsOut[3].maxs.x = maxs.x;
        rectsOut[3].mins.y = std::max(mins.y, second.maxs.y);
        rectsOut[3].maxs.y = maxs.y;
    }

    /// Attempt to merge this rectangle with another rectangle along a shared edge. If they can't be merged,
    /// return false. If they can be merged, sets merged to the result and returns true.
    bool merge(const Rect& other, Rect& merged) PRIME_NOEXCEPT
    {
        if (encloses(other)) {
            merged = *this;
            return true;
        }

        if (other.encloses(*this)) {
            merged = other;
            return true;
        }

        if (mins.x == other.mins.x && maxs.x == other.maxs.x && maxs.y >= other.mins.y && mins.y <= other.maxs.y) {
            merged.mins.x = mins.x;
            merged.maxs.x = maxs.x;
            merged.mins.y = std::min(mins.y, other.mins.y);
            merged.maxs.y = std::max(maxs.y, other.maxs.y);
            return true;
        }

        if (mins.y == other.mins.y && maxs.y == other.maxs.y && maxs.x >= other.mins.x && mins.x <= other.maxs.x) {
            merged.mins.y = mins.y;
            merged.maxs.y = maxs.y;
            merged.mins.x = std::min(mins.x, other.mins.x);
            merged.maxs.x = std::max(maxs.x, other.maxs.x);
            return true;
        }

        return false;
    }

    /// Construct a new rectangle that encloses both this rectangle and another.
    Rect getMerged(const Rect& other) const PRIME_NOEXCEPT
    {
        return Rect<Type>(mins.getMins(other.mins), maxs.getMaxs(other.maxs));
    }

    /// Modify our bounds to enclose the specified point.
    void enclose(const Point& point) PRIME_NOEXCEPT
    {
        enclose(point.x, point.y);
    }

    /// Modify our bounds to enclose the specified point.
    void enclose(Type x, Type y) PRIME_NOEXCEPT
    {
        if (x < mins.x)
            mins.x = x;
        if (x > maxs.x)
            maxs.x = x;

        if (y < mins.y)
            mins.y = y;
        if (y > maxs.y)
            maxs.y = y;
    }

    /// Return a Rect that has our coordinates extended to enclose the specified point.
    Rect getEnclosing(const Point& point) const PRIME_NOEXCEPT
    {
        return Rect<Type>(mins.getMins(point), maxs.getMaxs(point));
    }

    /// Return a Rect that has our coordinates extended to enclose the specified point.
    Rect getEnclosing(Type x, Type y) const PRIME_NOEXCEPT
    {
        return Rect<Type>(mins.getMins(x, y), maxs.getMaxs(x, y));
    }

    /// Returns a different kind of Rect, e.g., myRect.to<float>() returns a Rect<float>.
    template <typename Other>
    Rect<Other> to() const PRIME_NOEXCEPT
    {
        return Rect<Other>(mins.template to<Other>(), maxs.template to<Other>());
    }

    /// Returns a rectangle that contains the intersection of this rectangle and another.
    Rect getIntersection(const Rect& other) const PRIME_NOEXCEPT
    {
        return Rect<Type>(mins.getMaxs(other.mins), maxs.getMins(other.maxs));
    }

    /// Clamp a point to the interior of this rectangle.
    Point getClamped(const Point& point) const PRIME_NOEXCEPT
    {
        return Point(point.x < mins.x ? mins.x : (point.x > maxs.x ? maxs.x : point.x),
            point.y < mins.y ? mins.y : (point.y > maxs.y ? maxs.y : point.y));
    }
};

//
// Anything that can't go in the class declaration
//

template <typename Type>
inline typename Point<Type>::Dimensions Point<Type>::toDimensions() const PRIME_NOEXCEPT
{
    return Dimensions(x, y);
}
}

#endif
