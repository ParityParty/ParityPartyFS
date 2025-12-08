#pragma once

#include "disk/idisk.hpp"

class StackDisk : public IDisk {
    static constexpr size_t _size = 1048576; // 1MB
    std::uint8_t _data[_size] = {};

public:
    StackDisk() = default;

    size_t size() override;
    std::expected<void, FsError> read(size_t address, size_t size, buffer<uint8_t>& data) override;
    std::expected<size_t, FsError> write(size_t address, const buffer<uint8_t>& data) override;
};