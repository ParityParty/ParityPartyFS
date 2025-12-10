#pragma once

#include "disk/idisk.hpp"

class StackDisk : public IDisk {
    static constexpr size_t _size = 1 << 22; // 1MB
    std::uint8_t _data[_size] = {};

public:
    StackDisk() = default;

    size_t size() override;
    [[nodiscard]] virtual std::expected<std::vector<std::uint8_t>, FsError> read(
        size_t address, size_t size) override;
    [[nodiscard]] virtual std::expected<size_t, FsError> write(
        size_t address, const std::vector<std::uint8_t>& data) override;
};