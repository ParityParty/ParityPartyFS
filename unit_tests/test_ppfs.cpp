#include <gtest/gtest.h>

TEST(DummyTest, BasicAssertions)
{
    // Expect two strings to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}
