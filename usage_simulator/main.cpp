#include "data_collection/data_colection.hpp"
#include "disk/stack_disk.hpp"
#include "filesystem/ppfs.hpp"
#include "simulation/bit_flipper.hpp"
#include "simulation/mock_user.hpp"

#include <barrier>
#include <thread>

int main()
{
    Logger logger;
    StackDisk disk;
    PpFS fs(disk);
    if (!fs.format(FsConfig {
                       .total_size = disk.size(),
                       .average_file_size = 2000,
                       .block_size = 256,
                       .ecc_type = ECCType::ReedSolomon,
                       .rs_correctable_bytes = 3,
                       .use_journal = false,
                   })
            .has_value()) {
        std::cerr << "Failed to format disk" << std::endl;
        return 1;
    }
    SimpleBitFlipper flipper(disk, 0.5, 1, logger);
    std::vector<SingleDirMockUser> users;
    for (int i = 0; i < 200; i++) {
        auto dir = (std::stringstream() << "/user" << i).str();
        users.push_back(SingleDirMockUser(fs, logger,
            { .max_write_size = 500, .max_read_size = 500, .avg_steps_between_ops = 90 }, i, dir,
            i));
    }
    int iteration = 0;
    constexpr int MAX_ITERATIONS = 10000;
    auto on_completion = [&]() noexcept {
        logger.step();
        flipper.step();
        iteration++;
    };

    std::barrier barrier(users.size(), on_completion);
    auto work = [&](int id) {
        while (iteration < MAX_ITERATIONS) {
            users[id].step();
            barrier.arrive_and_wait();
        }
    };

    logger.step();
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