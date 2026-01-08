#pragma once

#include "disk/idisk.hpp"

#include <cstddef>
#include <cstdint>

class FileDisk final : public IDisk {
public:
    FileDisk(const char* path, size_t size);
    ~FileDisk() override;

    size_t size() override;

    [[nodiscard]] std::expected<void, FsError> read(
        size_t address, size_t size, static_vector<uint8_t>& data) override;

    [[nodiscard]] std::expected<size_t, FsError> write(
        size_t address, const static_vector<uint8_t>& data) override;

private:
    int _fd = -1;
    size_t _size = 0;
};
