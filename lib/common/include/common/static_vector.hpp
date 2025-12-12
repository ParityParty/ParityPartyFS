#include <cstddef>
#include <new>
#include <stdexcept>

template <typename T> class static_vector {
public:
    static_vector(T* buffer, std::size_t capacity, std::size_t size = 0)
        : _buffer(buffer)
        , _capacity(capacity)
        , _size(0)
    {
        if (_size > _capacity)
            throw std::bad_alloc();
    }

    void push_back(const T& value)
    {
        if (size_ >= capacity_)
            throw std::bad_alloc();
        new (buffer_ + size_++) T(value);
    }

    T& operator[](std::size_t i) { return buffer_[i]; }
    const T& operator[](std::size_t i) const { return buffer_[i]; }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }

    T* begin() { return buffer_; }
    T* end() { return buffer_ + size_; }

private:
    T* _buffer;
    std::size_t _capacity;
    std::size_t _size;
};