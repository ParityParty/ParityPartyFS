#pragma once
#include "idisk.hpp"

/**
 * Heap-allocated disk implementation with dynamic memory allocation.
 */
class HeapDisk : public IDisk {
    std::uint8_t* _buffer;
    size_t _size;

public:
    HeapDisk(size_t size);
    ~HeapDisk();

    [[nodiscard]] std::expected<void, FsError> read(
        size_t address, size_t size, static_vector<uint8_t>& data) override;
    ;
    [[nodiscard]] std::expected<size_t, FsError> write(
        size_t address, const static_vector<uint8_t>& data) override;

    size_t size() override;
};
