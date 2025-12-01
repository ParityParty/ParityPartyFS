#include "mock_user/mock_user.hpp"

#include <iostream>

MockUser::MockUser(IFilesystem& fs, const UserBehaviour behaviour)
    : _fs(fs)
    , _behaviour(behaviour)
{
}

void MockUser::step()
{
    if (_to_next_op > 0) {
        _to_next_op--;
        return;
    }

    _to_next_op = _behaviour.avg_steps_between_ops;
    std::cout << "Writing to file!" << std::endl;
    _fs.write(11, std::vector<std::uint8_t> { 1, 2, 3, 4 }, 88);
}