#pragma once

#include "simulation/mock_user.hpp"
#include <cstdint>
#include <string>

struct SimulationConfig {
    // Filesystem configuration
    uint32_t block_size = 256;
    ECCType ecc_type = ECCType::Hamming;
    uint32_t rs_correctable_bytes = 3;
    bool use_journal = false;

    // Bit flipper configuration
    double krad_per_year = 5;
    uint32_t bit_flip_seed = 1;

    // Mock user configuration
    uint32_t num_users = 10;
    UserBehaviour user_behaviour; // Now uses UserBehaviour with weights

    // Simulation configuration
    uint32_t simulation_seconds = 10000;
    uint32_t second_per_step = 1;
    std::string log_level = "Medium";

    /**
     * Load configuration from key=value file
     * @param filepath Path to configuration file
     * @return SimulationConfig with values from file or defaults
     */
    static SimulationConfig loadFromFile(const std::string& filepath);

    /**
     * Convert ECC type string to ECCType enum
     */
    static ECCType parseEccType(const std::string& ecc_str);

    /**
     * Convert ECCType enum to string
     */
    static std::string eccTypeToString(ECCType ecc);
};
