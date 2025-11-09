#include <gtest/gtest.h>
#include "filesystem/ppfs.hpp"

TEST(PpFS, Compiles) {
    PpFS fs();
    EXPECT_TRUE(true);
}