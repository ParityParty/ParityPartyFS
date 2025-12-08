#pragma once
#include "common/buffer.hpp"
#include "common/types.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <vector>

struct IDisk {
    virtual ~IDisk() = default;

    virtual std::expected<void, FsError> read(size_t address, size_t size, buffer<uint8_t>& data)
        = 0;
    virtual std::expected<size_t, FsError> write(size_t address, const buffer<std::uint8_t>& data)
        = 0;
    virtual size_t size() = 0;
};