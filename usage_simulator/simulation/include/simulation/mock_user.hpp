#pragma once
#include "data_collection/data_colection.hpp"
#include "filesystem/ifilesystem.hpp"

#include <random>

/**
 * Configuration for simulated user behavior.
 */
struct UserBehaviour {
    int max_write_size = 512;
    int max_read_size = 512;
    int avg_steps_between_ops = 10;
    int create_weight = 2;
    int write_weight = 10;
    int read_weight = 9;
    int delete_weight = 2;
};

/**
 * Represents a file or directory node in the simulated filesystem tree.
 */
struct FileNode {
    std::string name;
    bool is_dir;
    size_t size;
    std::vector<FileNode*> children;
};

/**
 * Mock user that performs filesystem operations in a single directory.
 */
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
    SingleDirMockUser(IFilesystem& fs, std::shared_ptr<Logger> logger,
        const UserBehaviour& behaviour, std::uint8_t id, std::string_view dir, unsigned int seed);
    /**
     * Executes one simulation step, performing filesystem operations based on configured behavior.
     */
    void step();
};