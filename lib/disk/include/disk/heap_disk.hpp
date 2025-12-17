#pragma once
#include "idisk.hpp"

class HeapDisk : public IDisk {
    std::uint8_t* _buffer;
    size_t _size;

public:
    HeapDisk(size_t size);
    ~HeapDisk();
    std::expected<std::vector<std::uint8_t>, FsError> read(size_t address, size_t size) override;
    std::expected<size_t, FsError> write(
        size_t address, const std::vector<std::uint8_t>& data) override;
    size_t size() override;
};
