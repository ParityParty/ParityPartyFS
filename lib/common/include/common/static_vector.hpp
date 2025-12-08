#pragma once

#include <array>
#include <vector>

#include "buffer.hpp"

/**
 * Static allocator used by static vector.
 */
template <typename T, std::size_t N> struct static_allocator {
    using value_type = T;

    T* storage = nullptr;
    std::size_t used = 0;

    static_allocator() = default;
    explicit static_allocator(T* external_storage)
        : storage(external_storage)
    {
    }

    T* allocate(std::size_t n)
    {
        if (!storage)
            throw std::runtime_error("static_allocator: storage not set");
        if (used + n > N)
            throw std::bad_alloc();
        T* ptr = storage + used;
        used += n;
        return ptr;
    }

    void deallocate(T*, std::size_t n) { used -= n }
};

/**
 * This class is designed to be used similar to std::vector,
 * but does not use dynamic memory. Data is stored in array structure
 * of capacity declared in the constructor.
 * Inherits publicly from std::vector with static_allocator to expose
 * the same interface.
 */
template <typename T, std::size_t max_size>
class static_vector : public buffer<T>, public std::vector<T, static_allocator<T, max_size>> {
public:
    using base_type = std::vector<T, static_allocator<T, max_size>>;

    using base_type::begin;
    using base_type::end;
    using base_type::insert;

    explicit static_vector(std::size_t max_cap, std::size_t initial_size = 0)
        : _alloc(_buffer.data())
        , _max(max_cap)
        , base_type(_alloc)
    {
        if (max_cap > max_size)
            throw std::runtime_error("static_vector: max_cap too large");
        this->resize(initial_size);
    }

    T& operator[](size_t idx) override { return base_type::operator[](idx); }
    const T& operator[](size_t idx) const override { return base_type::operator[](idx); }

    size_t size() const override { return base_type::size(); }
    size_t max_size() const override { return _max; }

    void push_back(const T& value) override { base_type::push_back(value); }

    T* data() override { return base_type::data(); }
    const T* data() const override { return base_type::data(); }

    T* begin() override { return base_type::begin(); }
    T* end() override { return base_type::end(); }
    const T* begin() const override { return base_type::begin(); }
    const T* end() const override { return base_type::end(); }

private:
    std::array<T, max_size> _buffer;
    static_allocator<T, max_size> _alloc;
    std::size_t _max;
};