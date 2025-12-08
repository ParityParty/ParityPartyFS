#pragma once

#include <array>
#include <vector>

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

    void deallocate(T*, std::size_t) { }
};

/**
 * This class is designed to be used similar to std::vector,
 * but does not use dynamic memory. Data is stored in array structure
 * of capacity declared in the constructor.
 * Inherits publicly from std::vector with static_allocator to expose
 * the same interface.
 */
template <typename T, std::size_t max_size>
class static_vector : public buffer_base<T>, public std::vector<T, static_allocator<T, max_size>> {
public:
    using base_type = std::vector<T, static_allocator<T, max_size>>;

    explicit static_vector(std::size_t max_cap, std::size_t initial_size = 0)
        : _alloc(_buffer.data())
        , _max(max_cap)
        , base_type(_alloc)
    {
        if (max_cap > max_size)
            throw std::runtime_error("static_vector: max_cap too large");
        this->resize(initial_size);
    }

    size_t max_size() { return _max; }

private:
    std::array<T, max_size> _buffer;
    static_allocator<T, max_size> _alloc;
    std::size_t _max;
};