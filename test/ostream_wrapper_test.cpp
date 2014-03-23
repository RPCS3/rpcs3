#include "yaml-cpp/ostream_wrapper.h"

#include "gtest/gtest.h"

namespace {
class OstreamWrapperTest : public ::testing::Test {
 protected:
};

// Tests that the Foo::Bar() method does Abc.
TEST_F(OstreamWrapperTest, ConstructNoWrite) {
  YAML::ostream_wrapper wrapper;
  EXPECT_STREQ("", wrapper.str());
}
}
