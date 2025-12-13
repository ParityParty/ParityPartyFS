#include "simulation/bit_flipper.hpp"

#include "common/bit_helpers.hpp"

SimpleBitFlipper::SimpleBitFlipper(
    IDisk& disk, float flip_chance, unsigned int seed, std::shared_ptr<Logger> logger)
    : _disk(disk)
    , _flip_chance(flip_chance)
    , _rng(seed)
    , _logger(logger)
{
}

void SimpleBitFlipper::step()
{
    std::bernoulli_distribution flip_dist(_flip_chance);
    if (flip_dist(_rng)) {
        std::uniform_int_distribution<int> location_dist(0, _disk.size() - 1);
        auto pos = location_dist(_rng);

        auto read_ret = _disk.read(pos, 1);
        if (!read_ret.has_value()) {
            return;
        }
        std::uniform_int_distribution<int> bit_dist(0, 7);
        auto bit_pos = bit_dist(_rng);
        BitHelpers::setBit(
            read_ret.value(), bit_pos, !BitHelpers::getBit(read_ret.value(), bit_pos));
        if (!_disk.write(pos, read_ret.value()).has_value()) {
            return;
        };
        _logger->logEvent(BitFlipEvent(pos));
    }
}
