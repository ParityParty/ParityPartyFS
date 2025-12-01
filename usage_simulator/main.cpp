#include "data_collection/data_colection.hpp"
#include "disk/stack_disk.hpp"
#include "filesystem/fs_mock.hpp"
#include "simulation/bit_flipper.hpp"
#include "simulation/mock_user.hpp"

int main()
{
    FsMock fs;
    Logger logger;
    MockUser user(fs, logger, { .max_write_size = 100, .avg_steps_between_ops = 30 }, 1);
    StackDisk disk;
    SimpleBitFlipper flipper(disk, 0.01, 1, logger);

    for (int i = 0; i < 1000; i++) {
        logger.step();
        flipper.step();
        user.step();
    }
    return 0;
}