/**
 * tests/test_addition.cpp
*/

#include "gtest/gtest.h"

namespace mylibrary {
    namespace tests {
        namespace test_addition {
            TEST(TestAddition, BasicTest) {
                ASSERT_TRUE(2 + 2 == 4);
            }
        }
    }
}

int main(int nArgs, char** vArgs) {
    ::testing::InitGoogleTest(&nArgs, vArgs);
    return RUN_ALL_TESTS();
}
