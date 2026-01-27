#include "simulation/irradiated_disk.hpp"

#include "common/bit_helpers.hpp"
#include "data_collection/data_colection.hpp"

#include <algorithm>
#include <ranges>

IrradiatedDisk::IrradiatedDisk(
    size_t size, IrradiationConfig config, std::shared_ptr<Logger> logger)
    : _size(size)
    , _buffer(new std::uint8_t[size])
    , _config(config)
    , _krad(0)
    , _logger(logger)
    , _rng(config.seed)
{
}

IrradiatedDisk::~IrradiatedDisk() { delete[] _buffer; }
std::expected<void, FsError> IrradiatedDisk::read(
    size_t address, size_t size, static_vector<uint8_t>& data)
{
    if (address > _size || address + size > _size) {
        return std::unexpected { FsError::Disk_OutOfBounds };
    }
    data.resize(size);
    std::memcpy(data.data(), _buffer + address, data.size());
    return { };
}

std::expected<size_t, FsError> IrradiatedDisk::write(
    size_t address, const static_vector<uint8_t>& data)
{
    if (address > _size || address + data.size() > _size) {
        return std::unexpected { FsError::Disk_OutOfBounds };
    }
    auto stuck_bits = _selectStuckBits(address, address + data.size());

    std::vector<bool> old_vals;
    for (auto& stuck_bit : stuck_bits) {
        old_vals.push_back(BitHelpers::getBit(_buffer, stuck_bit));
    }

    std::memcpy(_buffer + address, data.data(), data.size());

    for (int i = 0; i < stuck_bits.size(); i++) {
        if (_logger && BitHelpers::getBit(_buffer, stuck_bits[i]) != old_vals[i]) {
            _logger->logMsg("Stuck bit! bit failed to be overwritten");
        }
        BitHelpers::setBit(_buffer, stuck_bits[i], old_vals[i]);
    }

    return { };
}

size_t IrradiatedDisk::size() { return _size; }

void IrradiatedDisk::step()
{
    _krad += _config.krad_per_step;

    _nextFlips();

    // We start with next flips not to unflip any new bits
    _firstFlip();
}

std::uint64_t IrradiatedDisk::_selectNewBit()
{
    std::uint64_t bit_count = _size * 8;

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

void IrradiatedDisk::_flipNewBit()
{
    std::uint64_t new_bit = _selectNewBit();
    _flip(new_bit);
    _fragile_bits.insert(new_bit);
    if (_logger) {
        _logger->logMsg("New fragile bit");
    }
}

std::vector<size_t> IrradiatedDisk::_selectStuckBits(size_t from, size_t to)
{
    const double stuck_bit_prob = _config.delta * _krad + _config.zeta;

    auto amount_dist = std::binomial_distribution<size_t>(to - from, stuck_bit_prob);

    size_t num_stuck_bits = amount_dist(_rng);

    auto range = std::views::iota(from, to);
    std::vector<size_t> stuck_bits;
    stuck_bits.resize(num_stuck_bits);
    std::ranges::sample(range.begin(), range.end(), stuck_bits.begin(), num_stuck_bits, _rng);
    return stuck_bits;
}

void IrradiatedDisk::_firstFlip()
{
    const double expected_error_rate = std::exp(_config.alpha * _krad + _config.beta);

    const auto num_new_bits
        = static_cast<size_t>(expected_error_rate * _size * 8) - _fragile_bits.size();

    for (int i = 0; i < num_new_bits; i++) {
        _flipNewBit();
    }
}

void IrradiatedDisk::_nextFlips()
{
    double not_flip_prob = std::exp(-_config.gamma * _config.krad_per_step);
    auto flip_dist = std::bernoulli_distribution(1 - not_flip_prob);

    for (auto& fragile_bit : _fragile_bits) {
        if (flip_dist(_rng)) {
            _flip(fragile_bit);
            _logger->logMsg("Flipping fragile bit");
        }
    }
}

void IrradiatedDisk::_flip(const std::uint64_t pos) const
{
    BitHelpers::setBit(_buffer, pos, !BitHelpers::getBit(_buffer, pos));

    if (_logger) {
        _logger->logEvent(BitFlipEvent(pos / 8));
    }
}
