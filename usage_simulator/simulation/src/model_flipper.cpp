#include "simulation/model_flipper.hpp"

#include "common/bit_helpers.hpp"

constexpr double SECONDS_IN_YEAR = 365 * 25 * 60 * 60;

ModelFlipper::ModelFlipper(
    IDisk& disk, const ModelFlipperConfig& config, std::shared_ptr<Logger> logger)
    : _disk(disk)
    , _krad(0)
    , _krad_per_step(config.seconds_per_step * config.krad_per_year / SECONDS_IN_YEAR)
    , _rng(config.seed)
    , _logger(logger)
    , _alpha(config.alpha)
    , _beta(config.beta)
    , _gamma(config.gamma)
{
}

void ModelFlipper::step()
{
    _krad += _krad_per_step;

    _nextFlips();
    // First flips are second not to unflip the new bit
    _firstFlip();
}

std::uint64_t ModelFlipper::_selectNewBit()
{
    std::uint64_t bit_count = _disk.size() * 8;

    // We have to exclude the already fragile bits
    auto dist = std::uniform_int_distribution<std::uint64_t>(0, bit_count - _fragile_bits.size());
    std::uint64_t new_bit = dist(_rng);

    for (auto& old_bit : _fragile_bits) {
        if (new_bit < old_bit) {
            break;
        }
        new_bit += 1;
    }
    return new_bit;
}

void ModelFlipper::_firstFlip()
{
    double expected_error_rate = std::exp(_alpha * _krad + _beta);
    if (expected_error_rate * _disk.size() * 8 <= _fragile_bits.size()) {
        return;
    }

    std::uint64_t new_bit = _selectNewBit();
    auto ret = _flip(new_bit);
    if (!ret.has_value()) {
        _logger->logError("Couldn't flip a bit");
    }
    _fragile_bits.insert(new_bit);
    if (_logger) {
        _logger->logMsg("New fragile bit");
    }
}

void ModelFlipper::_nextFlips()
{
    double not_flip_prob = std::exp(-_gamma * _krad_per_step);
    auto flip_dist = std::bernoulli_distribution(1 - not_flip_prob);

    for (auto& fragile_bit : _fragile_bits) {
        if (flip_dist(_rng)) {
            auto ret = _flip(fragile_bit);
            if (!ret.has_value()) {
                _logger->logError("Couldn't flip a bit");
            }
            _logger->logMsg("Flipping fragile bit");
        }
    }
}

std::expected<void, FsError> ModelFlipper::_flip(const std::uint64_t pos) const
{
    std::uint64_t byte = pos / 8;

    std::uint8_t buffer[1];
    auto data = static_vector<std::uint8_t>(buffer, 1, 0);
    auto read_ret = _disk.read(byte, 1, data);
    if (!read_ret.has_value()) {
        return std::unexpected(read_ret.error());
    }

    auto bit_pos = pos % 8;

    BitHelpers::setBit(data, bit_pos, !BitHelpers::getBit(data, bit_pos));
    auto write_ret = _disk.write(byte, data);
    if (!write_ret.has_value()) {
        return std::unexpected(write_ret.error());
    }
    if (_logger) {
        _logger->logEvent(BitFlipEvent(byte));
    }
    return { };
}
