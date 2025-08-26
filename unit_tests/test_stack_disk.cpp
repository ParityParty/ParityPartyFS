#include <gtest/gtest.h>
#include "disk/stack_disk.hpp"

TEST(StackDisk, Compiles)
{
    StackDisk stack_disk;
    auto res = stack_disk.read(0,1);
    SUCCEED();
}
