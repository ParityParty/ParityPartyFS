#pragma once

#include "disk/idisk.hpp"

class StackDisk : public IDisk {
    static constexpr size_t _size = 1048576; // 1MB
    std::byte _data[_size] = {};

public:
    StackDisk() = default;

    size_t size() override;
    std::expected<std::vector<std::byte>, DiskError> read(size_t address, size_t size) override;
    std::expected<size_t, DiskError> write(
        size_t address, const std::vector<std::byte>& data) override;
};