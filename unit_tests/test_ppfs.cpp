#include <gtest/gtest.h>
#include "ppfs/ppfs.hpp"
#include "disk/stack_disk.hpp"

TEST(PpFS, Compiles)
{
    StackDisk disk;
    PpFS fs(disk);
    SUCCEED();
}
