#pragma once

#include "disk/idisk.hpp"
#include "simulation/bit_flipper.hpp"

#include <set>

struct ModelFlipperConfig {
    double krad_per_year;
    std::uint32_t seconds_per_step;
    std::uint32_t seed;
    double alpha;
    double beta;
    double gamma;
};

class ModelFlipper : public IBitFlipper {
    // Bits that heve already been flipped
    std::set<std::uint64_t> _fragile_bits;

    double _krad_per_step;
    double _krad;
    double _alpha;
    double _beta;
    double _gamma;

    IDisk& _disk;
    std::mt19937 _rng;
    std::shared_ptr<Logger> _logger;

    std::uint64_t _selectNewBit();
    void _firstFlip();
    void _nextFlips();
    std::expected<void, FsError> _flip(std::uint64_t pos) const;

public:
    ModelFlipper(
        IDisk& disk, const ModelFlipperConfig& config, std::shared_ptr<Logger> logger = nullptr);
    void step() override;
};
