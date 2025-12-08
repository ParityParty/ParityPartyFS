#include "data_collection/data_colection.hpp"
#include "disk/stack_disk.hpp"
#include "filesystem/ppfs.hpp"
#include "simulation/bit_flipper.hpp"
#include "simulation/mock_user.hpp"

int main()
{
    Logger logger;
    StackDisk disk;
    PpFS fs(disk);
    if (!fs.format(FsConfig {
                       .total_size = disk.size(),
                       .average_file_size = 2000,
                       .block_size = 512,
                       .ecc_type = ECCType::ReedSolomon,
                       .rs_correctable_bytes = 2,
                       .use_journal = false,
                   })
            .has_value()) {
        std::cerr << "Failed to format stack disk" << std::endl;
        return 1;
    }
    SimpleBitFlipper flipper(disk, 0.005, 1, logger);
    SingleDirMockUser user(
        fs, logger, { .max_write_size = 4000, .avg_steps_between_ops = 30 }, 0, "/user0", 0);

    for (int i = 0; i < 1000; i++) {
        logger.step();
        flipper.step();
        user.step();
    }
    return 0;
}