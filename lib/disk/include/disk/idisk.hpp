#pragma once
#include "common/static_vector.hpp"
#include "common/types.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>

struct IDisk {
    virtual ~IDisk() = default;

    [[nodiscard]] virtual std::expected<void, FsError> read(
        size_t address, size_t size, static_vector<uint8_t>& data)
        = 0;
    [[nodiscard]] virtual std::expected<size_t, FsError> write(size_t address, const static_vector<uint8_t>& data)
        = 0;
    virtual size_t size() = 0;
};