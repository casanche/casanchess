#include "test-Common.h"

int main(int argc, char** argv) {
    TestCommon::InitEngine();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
