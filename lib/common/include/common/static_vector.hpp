#pragma once

#include <cstddef>
#include <cstring>
#include <expected>
#include <new>
#include <stdexcept>

#include "common/types.hpp"

/**
 * @brief A fixed-capacity, stack-allocated vector-like container.
 *
 * `static_vector` is a template container that provides vector-like functionality
 * with a fixed capacity determined at construction or initialization time.
 * It stores elements in pre-allocated buffer memory (provided by the user or default),
 * making it suitable for embedded or real-time systems where dynamic allocation is undesirable.
 *
 * @tparam T The type of elements stored in the vector. Must be trivially copyable.
 *
 * @note This container only supports trivially copyable types to ensure memcpy-based operations.
 * @note Unlike std::vector, capacity is fixed and cannot grow beyond the initial buffer size.
 */
template <typename T> class static_vector {
public:
    /**
     * @brief Constructs an empty static_vector with no buffer.
     */
    static_vector()
        : _buffer(nullptr)
        , _capacity(0)
        , _size(0)
    {
    }

    /**
     * @brief Constructs a static_vector with an external buffer.
     * @param buffer Pointer to pre-allocated memory to use as the underlying storage.
     * @param capacity Maximum number of elements that can be stored.
     * @param size Initial number of valid elements (must not exceed capacity).
     */
    static_vector(T* buffer, std::size_t capacity, std::size_t size = 0)
        : _buffer(buffer)
        , _capacity(capacity)
        , _size(std::min(size, capacity))
    {
    }

    /**
     * @brief Appends an element to the end of the vector.
     * @param value Reference to the element to append.
     * @return void on success, or StaticVector_AllocationError if at capacity.
     */
    std::expected<void, FsError> push_back(const T& value)
    {
        if (_size >= _capacity)
            return std::unexpected(FsError::StaticVector_AllocationError);

        _buffer[_size++] = value;
        return {};
    }

    /**
     * @brief Accesses an element by index without bounds checking.
     * @param i Index of the element.
     * @return Reference to the element at index i.
     */
    T& operator[](std::size_t i) { return _buffer[i]; }
    /**
     * @brief Accesses an element by index without bounds checking (const version).
     * @param i Index of the element.
     * @return Const reference to the element at index i.
     */
    const T& operator[](std::size_t i) const { return _buffer[i]; }

    /**
     * @brief Returns the number of elements currently in the vector.
     * @return Number of valid elements.
     */
    std::size_t size() const { return _size; }
    /**
     * @brief Returns the maximum number of elements that can be stored.
     * @return The capacity of the vector.
     */
    std::size_t capacity() const { return _capacity; }
    /**
     * @brief Checks if the vector is empty.
     * @return true if size is 0, false otherwise.
     */
    bool empty() const { return _size == 0; }

    /**
     * @brief Returns an iterator to the first element.
     * @return Pointer to the first element.
     */
    T* begin() { return _buffer; }
    /**
     * @brief Returns an iterator to one past the last element.
     * @return Pointer to one past the last element.
     */
    T* end() { return _buffer + _size; }
    /**
     * @brief Returns a const iterator to the first element.
     * @return Const pointer to the first element.
     */
    const T* begin() const { return _buffer; }
    /**
     * @brief Returns a const iterator to one past the last element.
     * @return Const pointer to one past the last element.
     */
    const T* end() const { return _buffer + _size; }

    /**
     * @brief Returns a pointer to the underlying data buffer.
     * @return Pointer to the first element.
     */
    T* data() { return _buffer; }
    /**
     * @brief Returns a const pointer to the underlying data buffer.
     * @return Const pointer to the first element.
     */
    const T* data() const { return _buffer; }

    /**
     * @brief Resizes the vector to contain exactly the specified number of elements.
     * @param size The new size.
     * @return void on success, or StaticVector_AllocationError if size exceeds capacity.
     */
    std::expected<void, FsError> resize(size_t size)
    {
        if (size > _capacity)
            return std::unexpected(FsError::StaticVector_AllocationError);
        _size = size;
        return {};
    }

    /**
     * @brief Replaces the contents with elements from an initializer list.
     * @param init Initializer list of elements to copy.
     * @return void on success, or StaticVector_AllocationError if initializer list size exceeds
     * capacity.
     */
    std::expected<void, FsError> assign(std::initializer_list<T> init)
    {
        if (init.size() > _capacity)
            return std::unexpected(FsError::StaticVector_AllocationError);

        std::memcpy(_buffer, init.begin(), init.size() * sizeof(T));
        _size = init.size();

        return {};
    }

private:
    T* _buffer;
    std::size_t _capacity;
    std::size_t _size;

    static_assert(
        std::is_trivially_copyable_v<T>, "static_vector supports only trivially copyable types");
};