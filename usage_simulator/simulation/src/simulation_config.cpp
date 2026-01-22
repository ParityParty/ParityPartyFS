#include "simulation/simulation_config.hpp"
#include "blockdevice/ecc_type.hpp"
#include <fstream>
#include <iostream>

SimulationConfig SimulationConfig::loadFromFile(const std::string& filepath)
{
    SimulationConfig config;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file " << filepath << ", using defaults"
                  << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse key=value
        size_t delim = line.find('=');
        if (delim == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, delim);
        std::string value = line.substr(delim + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Parse filesystem config
        if (key == "block_size") {
            config.block_size = std::stoul(value);
        } else if (key == "ecc_type") {
            config.ecc_type = parseEccType(value);
        } else if (key == "rs_correctable_bytes") {
            config.rs_correctable_bytes = std::stoul(value);
        } else if (key == "use_journal") {
            config.use_journal = (value == "true" || value == "1");
        }
        // Parse bit flipper config
        else if (key == "krad_per_year") {
            config.krad_per_year = std::stod(value);
        } else if (key == "bit_flip_seed") {
            config.bit_flip_seed = std::stoul(value);
        }
        // Parse user config
        else if (key == "num_users") {
            config.num_users = std::stoul(value);
        } else if (key == "max_write_size") {
            config.user_behaviour.max_write_size = std::stoi(value);
        } else if (key == "max_read_size") {
            config.user_behaviour.max_read_size = std::stoi(value);
        } else if (key == "avg_steps_between_ops") {
            config.user_behaviour.avg_steps_between_ops = std::stoi(value);
        } else if (key == "create_weight") {
            config.user_behaviour.create_weight = std::stoi(value);
        } else if (key == "write_weight") {
            config.user_behaviour.write_weight = std::stoi(value);
        } else if (key == "read_weight") {
            config.user_behaviour.read_weight = std::stoi(value);
        } else if (key == "delete_weight") {
            config.user_behaviour.delete_weight = std::stoi(value);
        }
        // Parse simulation config
        else if (key == "simulation_years") {
            config.simulation_years = std::stoi(value);
        } else if (key == "seconds_per_step") {
            config.second_per_step = std::stoul(value);
        } else if (key == "log_level") {
            if (value == "None" || value == "none") {
                config.log_level = Logger::LogLevel::None;
            } else if (value == "Error" || value == "error") {
                config.log_level = Logger::LogLevel::Error;
            } else if (value == "Medium" || value == "medium") {
                config.log_level = Logger::LogLevel::Medium;
            } else if (value == "All" || value == "all") {
                config.log_level = Logger::LogLevel::All;
            }
        }
    }

    file.close();
    return config;
}

ECCType SimulationConfig::parseEccType(const std::string& ecc_str)
{
    if (ecc_str == "None" || ecc_str == "none") {
        return ECCType::None;
    }
    if (ecc_str == "Parity" || ecc_str == "parity") {
        return ECCType::Parity;
    }
    if (ecc_str == "Crc" || ecc_str == "CRC" || ecc_str == "crc") {
        return ECCType::Crc;
    }
    if (ecc_str == "Hamming" || ecc_str == "hamming") {
        return ECCType::Hamming;
    }
    if (ecc_str == "ReedSolomon" || ecc_str == "reed-solomon" || ecc_str == "RS") {
        return ECCType::ReedSolomon;
    }
    return ECCType::Hamming; // Default
}

std::string SimulationConfig::eccTypeToString(ECCType ecc)
{
    switch (ecc) {
    case ECCType::None:
        return "None";
    case ECCType::Parity:
        return "Parity";
    case ECCType::Crc:
        return "CRC";
    case ECCType::Hamming:
        return "Hamming";
    case ECCType::ReedSolomon:
        return "ReedSolomon";
    }
    return "Unknown";
}
