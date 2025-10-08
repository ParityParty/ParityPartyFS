#pragma once
#include <cstddef>
#include <expected>
#include <string_view>
#include <vector>

enum class DiskError {
    IOError,
    OutOfBounds,
    InvalidRequest,
    InternalError,
    OutOfMemory,

};

inline std::string_view toString(DiskError err)
{
    switch (err) {
    case DiskError::IOError:
        return "IOError";
    case DiskError::OutOfBounds:
        return "OutOfBounds";
    case DiskError::InvalidRequest:
        return "InvalidRequest";
    case DiskError::InternalError:
        return "InternalError";
    case DiskError::OutOfMemory:
        return "OutOfMemory";
    default:
        return "UnknownError";
    }
}

struct IDisk {
    virtual ~IDisk() = default;

    virtual std::expected<std::vector<std::byte>, DiskError> read(size_t address, size_t size) = 0;
    virtual std::expected<size_t, DiskError> write(
        size_t address, const std::vector<std::byte>& data)
        = 0;
    virtual size_t size() = 0;
};