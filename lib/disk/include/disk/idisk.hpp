#pragma once
#include "common/types.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string_view>
#include <vector>

struct IDisk {
    virtual ~IDisk() = default;

    virtual std::expected<std::vector<std::uint8_t>, FsError> read(size_t address, size_t size) = 0;
    virtual std::expected<size_t, FsError> write(
        size_t address, const std::vector<std::uint8_t>& data)
        = 0;
    virtual size_t size() = 0;
};