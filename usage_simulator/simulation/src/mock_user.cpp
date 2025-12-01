#include "simulation/mock_user.hpp"

#include <chrono>

MockUser::MockUser(IFilesystem& fs, Logger& logger, UserBehaviour behaviour, unsigned int seed)
    : _fs(fs)
    , _logger(logger)
    , _behaviour(behaviour)
    , _rng(seed)
{
}

void MockUser::step()
{
    if (_to_next_op > 0) {
        _to_next_op--;
        return;
    }

    std::uniform_int_distribution<int> next_op_dist(1, 2 * _behaviour.avg_steps_between_ops);
    _to_next_op = next_op_dist(_rng);

    std::uniform_int_distribution<int> size_dist(1, _behaviour.max_write_size);
    auto write_size = size_dist(_rng);

    auto start = std::chrono::high_resolution_clock::now();
    _fs.write(11, std::vector<std::uint8_t>(write_size, 0x55), 88);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    _logger.log(WriteEvent(write_size, duration));
}
