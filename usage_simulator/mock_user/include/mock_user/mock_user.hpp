#pragma once
#include "data_collection/data_colection.hpp"
#include "filesystem/ifilesystem.hpp"

struct UserBehaviour {
    int max_write_size = 512;
    int max_read_size = 512;
    int avg_steps_between_ops = 10;
};

class MockUser {
    IFilesystem& _fs;
    Logger& _logger;
    UserBehaviour _behaviour;
    int _to_next_op = 0;

public:
    MockUser(IFilesystem& fs, Logger& logger, UserBehaviour behaviour);
    void step();
};