#pragma once
#include "filesystem/ifilesystem.hpp"

struct UserBehaviour {
    int max_write_size = 512;
    int max_read_size = 512;
    int avg_steps_between_ops = 10;
};

class MockUser {
    IFilesystem& _fs;
    UserBehaviour _behaviour;
    int _to_next_op = 0;

public:
    MockUser(IFilesystem& fs, UserBehaviour behaviour);
    void step();
};