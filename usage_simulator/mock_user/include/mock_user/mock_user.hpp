#pragma once
#include "filesystem/ifilesystem.hpp"

#include <memory>

struct UseArgs {
    int max_write_size;
    int max_read_size;
    float avg_ops_per_sec;
};

class FileNode {
public:
    bool is_dir;
    size_t size;
    std::vector<std::unique_ptr<FileNode>> children;
};

class MockUser {
    IFilesystem& _fs;
    std::unique_ptr<FileNode> _root;

public:
    MockUser(IFilesystem& fs);
    void use(UseArgs args);
};