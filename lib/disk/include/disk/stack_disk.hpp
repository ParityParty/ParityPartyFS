#pragma once

#include "disk/idisk.hpp"
#include <cstdint>
#include <cstring>
#include <expected>
#include <vector>

template <size_t power = 22> class StackDisk : public IDisk {
    static constexpr size_t _size = 1 << power;
    std::uint8_t _data[_size] = {};

public:
    StackDisk() = default;

    size_t size() override { return _size; }

    [[nodiscard]] std::expected<std::vector<std::uint8_t>, FsError> read(
        size_t address, size_t size) override
    {
        if (address + size > _size) {
            return std::unexpected(FsError::Disk_OutOfBounds);
        }
        // Construct vector from pointer range
        return std::vector<std::uint8_t>(_data + address, _data + address + size);
    }

    [[nodiscard]] std::expected<size_t, FsError> write(
        size_t address, const std::vector<std::uint8_t>& data) override
    {
        if (address + data.size() > _size) {
            return std::unexpected(FsError::Disk_OutOfBounds);
        }
        std::memcpy(_data + address, data.data(), data.size());
        return data.size();
    }
};