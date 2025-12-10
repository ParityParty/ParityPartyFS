#pragma once
#include "data_collection/data_colection.hpp"
#include "disk/idisk.hpp"

#include <random>

struct IBitFlipper {
    virtual ~IBitFlipper() = default;
    virtual void step() = 0;
};

class SimpleBitFlipper final : public IBitFlipper {
    IDisk& _disk;
    float _flip_chance;
    std::mt19937 _rng;
    std::shared_ptr<Logger> _logger;

public:
    SimpleBitFlipper(
        IDisk& disk, float flip_chance, unsigned int seed, std::shared_ptr<Logger> logger);
    void step() override;
};