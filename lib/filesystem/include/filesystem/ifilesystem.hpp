#pragma once
#include "disk/idisk.hpp"

#include <expected>

struct IFilesystem {
    std::expected<void, DiskError> format();
};
