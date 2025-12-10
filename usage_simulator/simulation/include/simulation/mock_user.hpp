#pragma once
#include "data_collection/data_colection.hpp"
#include "filesystem/ifilesystem.hpp"

#include <random>

struct UserBehaviour {
    int max_write_size = 512;
    int max_read_size = 512;
    int avg_steps_between_ops = 10;
};

struct FileNode {
    std::string name;
    bool is_dir;
    size_t size;
    std::vector<FileNode*> children;
};

class SingleDirMockUser {
    IFilesystem& _fs;
    std::shared_ptr<Logger> _logger;
    UserBehaviour _behaviour;

    std::string _dir;
    FileNode* _root = nullptr;
    int _to_next_op = 0;
    std::mt19937 _rng;
    std::uint32_t _file_id = 0;

    void _createFile();
    void _writeToFile();
    void _readFromFile();
    void _deleteFile();

public:
    const std::uint8_t id;
    SingleDirMockUser(IFilesystem& fs, std::shared_ptr<Logger> logger, UserBehaviour behaviour,
        std::uint8_t id, std::string_view dir, unsigned int seed);
    void step();
};