#pragma once
#include "filesystem/ifilesystem.hpp"

#include <memory>
#include <stop_token>

struct UseArgs {
    int max_write_size = 512;
    int max_read_size = 512;
    float avg_ops_per_sec = 2;
    std::stop_token token;
};

class MockUser {
    IFilesystem& _fs;

public:
    MockUser(IFilesystem& fs);
    void use(UseArgs args);
};