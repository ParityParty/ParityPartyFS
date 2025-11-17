#pragma once
#include "common/types.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string_view>
#include <vector>

inline std::string_view toString(FsError err)
{
    switch (err) {
    case FsError::IOError:
        return "IOError";
    case FsError::OutOfBounds:
        return "OutOfBounds";
    case FsError::InvalidRequest:
        return "InvalidRequest";
    case FsError::InternalError:
        return "InternalError";
    case FsError::OutOfMemory:
        return "OutOfMemory";
    default:
        return "UnknownError";
    }
}

struct IDisk {
    virtual ~IDisk() = default;

    virtual std::expected<std::vector<std::uint8_t>, FsError> read(size_t address, size_t size) = 0;
    virtual std::expected<size_t, FsError> write(
        size_t address, const std::vector<std::uint8_t>& data)
        = 0;
    virtual size_t size() = 0;
};