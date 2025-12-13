#pragma once

#include <cstddef>
#include <cstring>
#include <new>
#include <stdexcept>

template <typename T> class static_vector {
public:
    static_vector(T* buffer, std::size_t capacity, std::size_t size = 0)
        : _buffer(buffer)
        , _capacity(capacity)
        , _size(size)
    {
        if (_size > _capacity)
            throw std::bad_alloc();
    }

    void push_back(const T& value)
    {
        if (_size >= _capacity)
            throw std::bad_alloc();

        _buffer[_size++] = value;
    }

    T& operator[](std::size_t i) { return _buffer[i]; }
    const T& operator[](std::size_t i) const { return _buffer[i]; }

    std::size_t size() const { return _size; }
    std::size_t capacity() const { return _capacity; }

    T* begin() { return _buffer; }
    T* end() { return _buffer + _size; }
    const T* begin() const { return _buffer; }
    const T* end() const { return _buffer + _size; }

    T* data() { return _buffer; }
    const T* data() const { return _buffer; }

    void resize(size_t size)
    {
        if (size > _capacity)
            throw std::bad_alloc();
        _size = size;
    }

    void assign(std::initializer_list<T> init)
    {
        if (init.size() > _capacity)
            throw std::bad_alloc();

        std::memcpy(_buffer, init.begin(), init.size() * sizeof(T));
        _size = init.size();
    }

private:
    T* _buffer;
    std::size_t _capacity;
    std::size_t _size;

    static_assert(std::is_trivially_copyable_v<T>, 
        "static_vector supports only trivially copyable types");
};