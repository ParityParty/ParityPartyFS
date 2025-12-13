#pragma once

#include "disk/idisk.hpp"

class StackDisk : public IDisk {
    static constexpr size_t _size = 1 << 22; // 4MB
    std::uint8_t _data[_size] = {};

public:
    StackDisk() = default;

    size_t size() override;
    [[nodiscard]] std::expected<void, FsError> read(
        size_t address, size_t size, static_vector<uint8_t>& data) override;
    [[nodiscard]] std::expected<size_t, FsError> write(
        size_t address, const static_vector<uint8_t>& data) override;
};
