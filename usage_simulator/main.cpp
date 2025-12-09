#include "data_collection/data_colection.hpp"
#include "disk/stack_disk.hpp"
#include "filesystem/ppfs.hpp"
#include "simulation/bit_flipper.hpp"
#include "simulation/mock_user.hpp"

#include <execution>

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
    SimpleBitFlipper flipper(disk, 0.005, 1, logger);
    std::vector<SingleDirMockUser> users;
    for (int i = 0; i < 20; i++) {
        auto dir = (std::stringstream() << "/user" << i).str();
        users.push_back(SingleDirMockUser(fs, logger,
            { .max_write_size = 500, .max_read_size = 500, .avg_steps_between_ops = 50 }, i, dir,
            i));
    }

    for (int i = 0; i < 100000; i++) {
        logger.step();
        //        flipper.step();
        // std::for_each(
        //     std::execution::par, users.begin(), users.end(), [](auto& user) { user.step(); });
        for (auto& user : users) {
            user.step();
        }
    }
    return 0;
}