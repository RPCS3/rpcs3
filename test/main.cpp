#include "tests.h"

#include "gtest/gtest.h"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  Test::RunAll();
  return RUN_ALL_TESTS();
}
