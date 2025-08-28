#pragma once
#include <cstddef>
#include <expected>
#include <vector>

enum class DiskError {
    IOError,
    OutOfBounds,
    InvalidRequest,
    InternalError,
};

struct IDisk {
    virtual ~IDisk() = default;

    virtual std::expected<std::vector<std::byte>, DiskError> read(size_t address, size_t size) = 0;
    virtual std::expected<size_t, DiskError> write(size_t address, const std::vector<std::byte>& data) = 0;
    virtual size_t size() = 0;
};