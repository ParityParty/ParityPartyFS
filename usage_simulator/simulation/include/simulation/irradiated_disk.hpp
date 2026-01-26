#pragma once

#include "bit_flipper.hpp"
#include "disk/idisk.hpp"

#include <memory>
#include <random>
#include <set>

class Logger;
struct IrradiationConfig {
    double krad_per_step;
    std::uint32_t seed;
    double alpha;
    double beta;
    double gamma;
    double delta;
    double zeta;
};

class IrradiatedDisk : public IDisk, IBitFlipper {
    std::set<std::uint64_t> _fragile_bits;

    std::uint8_t* _buffer;
    size_t _size;
    double _krad;
    IrradiationConfig _config;
    std::shared_ptr<Logger> _logger;
    std::mt19937 _rng;

    std::uint64_t _selectNewBit();
    void _flipNewBit();

    // Returns stuck bits
    std::vector<size_t> _selectStuckBits(size_t from, size_t to);
    void _firstFlip();
    void _nextFlips();
    void _flip(std::uint64_t pos) const;

public:
    IrradiatedDisk(size_t size, IrradiationConfig config, std::shared_ptr<Logger> logger);
    ~IrradiatedDisk();

    [[nodiscard]] std::expected<void, FsError> read(
        size_t address, size_t size, static_vector<uint8_t>& data) override;
    [[nodiscard]] std::expected<size_t, FsError> write(
        size_t address, const static_vector<uint8_t>& data) override;
    size_t size() override;

    void step() override;
};