#include "data_collection/data_colection.hpp"
#include "disk/heap_disk.hpp"
#include "filesystem/ppfs.hpp"
#include "simulation/bit_flipper.hpp"
#include "simulation/mock_user.hpp"
#include "simulation/simulation_config.hpp"

#include <barrier>
#include <iostream>
#include <thread>
#include <unistd.h>

int main(int argc, char* argv[])
{
    // Detect if stdout is a TTY (terminal)
    bool is_tty = isatty(STDOUT_FILENO);

    SimulationConfig sim_config;

    if (argc > 1) {
        sim_config = SimulationConfig::loadFromFile(argv[1]);
        if (is_tty) {
            std::cout << "Configuration loaded from: " << argv[1] << std::endl;
        }
    } else {
        if (is_tty) {
            std::cout << "Usage: " << argv[0] << " <config_file> <logs_folder>" << std::endl;
            std::cout << "Using default configuration" << std::endl;
        }
    }

    // Set logger to None if not a TTY (being run by Python)
    Logger::LogLevel log_level = is_tty ? Logger::LogLevel::All : Logger::LogLevel::None;
    std::shared_ptr<Logger> logger = std::make_shared<Logger>(log_level, argv[2]);
    HeapDisk disk(1 << 25);
    PpFS fs(disk, logger);
    if (!fs.format(FsConfig {
                       .total_size = disk.size(),
                       .average_file_size = 2000,
                       .block_size = sim_config.block_size,
                       .ecc_type = sim_config.ecc_type,
                       .rs_correctable_bytes = sim_config.rs_correctable_bytes,
                       .use_journal = sim_config.use_journal,
                   })
            .has_value()) {
        std::cerr << "Failed to format disk" << std::endl;
        return 1;
    }

    SimpleBitFlipper flipper(disk, sim_config.krad_per_year / 100, sim_config.bit_flip_seed,
        logger); // Placeholder values

    std::vector<SingleDirMockUser> users;
    for (int i = 0; i < static_cast<int>(sim_config.num_users); i++) {
        auto dir = (std::stringstream() << "/user" << i).str();
        users.push_back(SingleDirMockUser(fs, logger, sim_config.user_behaviour, i, dir, i));
    }
    int iteration = 0;
    const int MAX_ITERATIONS = sim_config.simulation_seconds / sim_config.second_per_step;

    auto on_completion = [&]() noexcept {
        logger->step();
        flipper.step();
        iteration++;

        // Send progress updates to stdout if not a TTY (for Python to parse)
        if (!is_tty && iteration % 100 == 0) {
            std::cout << "PROGRESS:" << iteration << "/" << MAX_ITERATIONS << std::endl;
            std::cout.flush();
        }
    };

    std::barrier barrier(users.size(), on_completion);
    auto work = [&](int id) {
        while (iteration < MAX_ITERATIONS) {
            users[id].step();
            barrier.arrive_and_wait();
        }
    };

    logger->step();
    flipper.step();

    std::vector<std::jthread> threads;
    threads.reserve(std::size(users));
    for (auto const& user : users)
        threads.emplace_back(work, user.id);

    for (auto& thread : threads) {
        thread.join();
    }
    return 0;
}