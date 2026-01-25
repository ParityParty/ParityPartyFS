#pragma once
#include "ppfs/data_collection/data_colection.hpp"
#include "ppfs/disk/idisk.hpp"

#include <random>

/**
 * Interface for bit-flip simulation in disk storage.
 */
struct IBitFlipper {
    virtual ~IBitFlipper() = default;
    /**
     * Performs a simulation step, potentially introducing bit flips.
     */
    virtual void step() = 0;
};

/**
 * Simple implementation that randomly flips bits in disk storage.
 */
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