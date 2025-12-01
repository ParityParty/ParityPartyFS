#include "mock_user/mock_user.hpp"

#include <chrono>

MockUser::MockUser(IFilesystem& fs, Logger& logger, UserBehaviour behaviour)

    : _fs(fs)
    , _logger(logger)
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
    auto start = std::chrono::high_resolution_clock::now();
    _fs.write(11, std::vector<std::uint8_t> { 1, 2, 3, 4 }, 88);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    _logger.log(WriteEvent(4, duration));
}
