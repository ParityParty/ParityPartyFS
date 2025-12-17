#pragma once

#include "disk/idisk.hpp"
#include <cstdint>
#include <cstring>
#include <expected>
#include <vector>

static constexpr size_t DEFAULT_STACK_DISK_POWER = 22;

template <size_t power = DEFAULT_STACK_DISK_POWER> class StackDisk : public IDisk {
    static constexpr size_t _size = 1 << power;
    std::uint8_t _data[_size] = {};

public:
    StackDisk() = default;

    size_t size() override { return _size; }

    [[nodiscard]] std::expected<void, FsError> read(
        size_t address, size_t size, static_vector<uint8_t>& data)
    {
        if (address + size > _size) {
            return std::unexpected(FsError::Disk_OutOfBounds);
        }
        if (data.capacity() < size)
            return std::unexpected(FsError::Disk_InvalidRequest);

        data.resize(size);
        std::memcpy(data.data(), _data + address, data.size());
        return {};
    }

    [[nodiscard]] std::expected<size_t, FsError> write(
        size_t address, const static_vector<uint8_t>& data)
    {
        if (address + data.size() > _size) {
            return std::unexpected(FsError::Disk_OutOfBounds);
        }
        std::memcpy(_data + address, data.data(), data.size());
        return data.size();
    }
};
