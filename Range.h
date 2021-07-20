// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_RANGE_H
#define PRIME_RANGE_H

#include "Config.h"
#include <algorithm>

namespace Prime {

/// A [start, end) range (includes start, does not include end).
template <typename Type>
class Range {
public:
    /// Constructs an invalid range (start > end).
    Range() PRIME_NOEXCEPT : _start(Type(1)),
                             _end(0)
    {
    }

    Range(Type start, Type end) PRIME_NOEXCEPT : _start(start),
                                                 _end(end)
    {
    }

    Range(const Range& copy) PRIME_NOEXCEPT : _start(copy._start),
                                              _end(copy._end)
    {
    }

    /// Returns true if this range is valid (start <= end).
    bool isValid() const PRIME_NOEXCEPT { return _start <= _end; }

    /// Returns true if this range is not valid (start > end).
    bool isInvalid() const PRIME_NOEXCEPT { return _start > _end; }

    Type getStart() const PRIME_NOEXCEPT { return _start; }

    Type getEnd() const PRIME_NOEXCEPT { return _end; }

    Type getLength() const PRIME_NOEXCEPT { return _end - _start; }

    Type getMiddle() const PRIME_NOEXCEPT { return (_start + _end) / 2; }

    Range getReplacingStart(Type newStart) const PRIME_NOEXCEPT { return Range(newStart, _end); }

    Range getReplacingEnd(Type newEnd) const PRIME_NOEXCEPT { return Range(_start, newEnd); }

    Type clamp(Type value) const PRIME_NOEXCEPT
    {
        return Clamp(value, _start, _end);
    }

    Range<Type> clamp(const Range<Type>& range) const PRIME_NOEXCEPT
    {
        return Range<Type>(clamp(range.getStart()), clamp(range.getEnd()));
    }

    /// Returns a new Range that contains our range, extended to enclose the value.
    Range getEnclosing(Type value) const PRIME_NOEXCEPT
    {
        if (isInvalid()) {
            return Range(value, value);
        }

        if (value < _start) {
            return Range(value, _end);
        }

        if (value > _end) {
            return Range(_start, value);
        }

        return *this;
    }

    /// Returns a new Range that contains our range, extended to enclose another Range.
    Range getEnclosing(const Range& other) const PRIME_NOEXCEPT
    {
        if (other.isInvalid()) {
            return *this;
        }

        if (isInvalid()) {
            return other;
        }

        return Range(std::min(_start, other._start), std::max(_end, other._end));
    }

    bool contains(Type value) const PRIME_NOEXCEPT
    {
        return value >= _start && value < _end;
    }

    bool contains(const Range& range) const PRIME_NOEXCEPT
    {
        return range._start >= _start && range._end <= _end;
    }

    bool intersects(const Range& range) const PRIME_NOEXCEPT
    {
        return !(_end <= range._start || _start >= range._end);
    }

    bool operator==(const Range& rhs) const PRIME_NOEXCEPT { return _start == rhs._start && _end == rhs._end; }
    bool operator!=(const Range& rhs) const PRIME_NOEXCEPT { return !operator==(rhs); }

    bool isAlmostEqual(const Range& other, Type tolerance) const PRIME_NOEXCEPT
    {
        return Abs(_start - other._start) < tolerance && Abs(_end - other._end) < tolerance;
    }

    Range getSwappedIfInvalid() const PRIME_NOEXCEPT
    {
        if (isInvalid()) {
            return Range(_end, _start);
        }

        return *this;
    }

    Range<Type> operator+(const Type& offset) const PRIME_NOEXCEPT
    {
        return Range<Type>(_start + offset, _end + offset);
    }

private:
    Type _start;
    Type _end;
};

}

#endif
