#include "simulation/bit_flipper.hpp"
#include "common/bit_helpers.hpp"
#include "common/static_vector.hpp"

#include <array>

SimpleBitFlipper::SimpleBitFlipper(
    IDisk& disk, const float flip_chance, const unsigned int seed, std::shared_ptr<Logger> logger)
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
        std::uniform_int_distribution<size_t> location_dist(0, _disk.size() - 1);
        const auto pos = location_dist(_rng);

        std::array<uint8_t, 1> read_buf { };
        static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
        auto read_ret = _disk.read(pos, 1, read_data);
        if (!read_ret.has_value()) {
            return;
        }
        std::uniform_int_distribution<int> bit_dist(0, 7);
        auto bit_pos = bit_dist(_rng);
        BitHelpers::setBit(read_data, bit_pos, !BitHelpers::getBit(read_data, bit_pos));
        if (!_disk.write(pos, read_data).has_value()) {
            return;
        }
        _logger->logEvent(BitFlipEvent(pos));
    }
}
