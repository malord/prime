// Copyright 2000-2021 Mark H. P. Lord

// CircularQueue and MovingAverage

#ifndef PRIME_CIRCULARQUEUE_H
#define PRIME_CIRCULARQUEUE_H

#include "Config.h"

namespace Prime {

//
// CircularQueue
//

/// A fixed-capacity circular queue.
template <typename Type>
class CircularQueue {
public:
    template <unsigned int BufferSize>
    class Buffer;

    CircularQueue() PRIME_NOEXCEPT : _queue(0),
                                     _capacity(0),
                                     _read(0),
                                     _write(0),
                                     _empty(true),
                                     _delete(false)
    {
    }

    /// Supply an existing buffer to use.
    CircularQueue(Type* buffer, size_t buffer_size, bool deleteBuffer = false) PRIME_NOEXCEPT : _queue(buffer),
                                                                                                _capacity(buffer_size),
                                                                                                _read(0),
                                                                                                _write(0),
                                                                                                _empty(true),
                                                                                                _delete(deleteBuffer)
    {
    }

    ~CircularQueue() PRIME_NOEXCEPT
    {
        if (_delete) {
            delete[] _queue;
        }
    }

    /// If buffer is NULL then it will be allocated for you.
    void init(size_t capacity, Type* buffer = NULL, bool deleteBuffer = false)
    {
        reset();

        if (buffer) {
            _queue = buffer;
            _delete = deleteBuffer;
        } else {
            _queue = new Type[capacity];
            _delete = true;
        }

        _capacity = capacity;
    }

    /// Free the queue memory if we should, then clear the array.
    void reset() PRIME_NOEXCEPT
    {
        if (_delete) {
            _delete = false;

            delete[] _queue;
            _queue = NULL;
            _capacity = 0;
        }

        clear();
    }

    bool empty() const PRIME_NOEXCEPT { return _empty; }

    bool full() const PRIME_NOEXCEPT { return _read == _write && !_empty; }

    void clear() PRIME_NOEXCEPT
    {
        _write = _read = 0;
        _empty = true;
    }

    bool push_back(const Type& element)
    {
        if (full()) {
            return false;
        }

        _queue[_write] = element;
        _write = (_write + 1) % _capacity;
        _empty = false;

        return true;
    }

#ifdef PRIME_COMPILER_RVALUEREF
    bool push_back(Type&& element)
    {
        if (full()) {
            return false;
        }

        _queue[_write] = PRIME_MOVE(element);
        _write = (_write + 1) % _capacity;
        _empty = false;

        return true;
    }
#endif

    size_t size() const PRIME_NOEXCEPT
    {
        if (_empty) {
            return 0;
        }

        if (_write > _read) {
            return _write - _read;
        }

        return _capacity - (_read - _write);
    }

    const Type& front() const PRIME_NOEXCEPT
    {
        PRIME_ASSERT(!empty());
        return _queue[_read];
    }

    Type& front() PRIME_NOEXCEPT
    {
        PRIME_ASSERT(!empty());
        return _queue[_read];
    }

    template <typename Index>
    const Type& at(Index index) const PRIME_NOEXCEPT
    {
        PRIME_ASSERT((size_t)index < size());
        return _queue[(_read + index) % _capacity];
    }

    template <typename Index>
    Type& at(Index index) PRIME_NOEXCEPT
    {
        PRIME_ASSERT((size_t)index < size());
        return _queue[(_read + index) % _capacity];
    }

    template <typename Index>
    const Type& operator[](Index index) const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT((size_t)index < size());
        return _queue[(_read + index) % _capacity];
    }

    template <typename Index>
    Type& operator[](Index index) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT((size_t)index < size());
        return _queue[(_read + index) % _capacity];
    }

    Type& pop_front() PRIME_NOEXCEPT
    {
        PRIME_ASSERT(!empty());

        size_t prev = _read;
        _read = (_read + 1) % _capacity;
        _empty = (_read == _write);
        return _queue[prev];
    }

    /// Read an element from the queue, then remove it, shuffling all the remaining elements along.
    template <typename Index>
    Type remove(Index index)
    {
        PRIME_ASSERT((size_t)index < size());

        size_t where = (_read + (size_t)(index)) % _capacity;

        Type element = _queue[where];
        for (;;) {
            size_t next = (where + 1) % _capacity;
            if (next == _write) {
                _write = where;
                _empty = (_read == _write);
                return element;
            }

            _queue[where] = _queue[next];
            where = next;
        }
    }

    CircularQueue& operator=(const CircularQueue& rhs)
    {
        if (this == &rhs) {
            return *this;
        }

        clear();
        size_t rhs_size = rhs.size();
        for (size_t i = 0; i != rhs_size; ++i) {
            push_back(rhs[i]);
        }

        return *this;
    }

private:
    Type* _queue;
    size_t _capacity;
    size_t _read;
    size_t _write;
    bool _empty;
    bool _delete;

    CircularQueue(const CircularQueue&) { }
};

/// A CircularQueue that contains the array, e.g., CircularQueue<std::string>::Buffer<32> has internal storage
/// for 32 items.
template <typename Type>
template <unsigned int BufferSize>
class CircularQueue<Type>::Buffer : public CircularQueue<Type> {
public:
    Buffer() PRIME_NOEXCEPT : CircularQueue<Type>(_buffer, BufferSize)
    {
    }

private:
    Type _buffer[BufferSize];

    PRIME_UNCOPYABLE(Buffer)
};

//
// MovingAverage
//

/// e.g., MovingAverage<float, CircularQueue<float>::Buffer<32> >
template <typename Type, typename Queue = CircularQueue<Type>, typename ScalarType = Type>
class MovingAverage {
public:
    MovingAverage() PRIME_NOEXCEPT : _total(0)
    {
    }

    /// Allocate a buffer or, if buffer is non-null, use an existing buffer.
    void init(size_t capacity, Type* buffer = NULL, bool deleteBuffer = false)
    {
        _queue.init(capacity, buffer, deleteBuffer);
    }

    void clear() PRIME_NOEXCEPT
    {
        _total = Type(0);
        _queue.clear();
    }

    void write(Type value)
    {
        if (_queue.full()) {
            _total -= _queue.pop_front();
        }

        _queue.push_back(value);
        _total += value;
    }

    bool empty() const PRIME_NOEXCEPT { return _queue.empty(); }

    bool full() const PRIME_NOEXCEPT { return _queue.full(); }

    size_t size() const PRIME_NOEXCEPT { return _queue.size(); }

    Type total() const { return _total; }

    Type get() const
    {
        if (!PRIME_DEBUG_GUARD(!empty())) {
            return Type(0);
        }

        return _total / Type(ScalarType(size()));
    }

private:
    Queue _queue;
    Type _total;

    PRIME_UNCOPYABLE(MovingAverage);
};
}

#endif
