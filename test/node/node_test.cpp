#include "gtest/gtest.h"
#include "yaml-cpp/node/node.h"

namespace YAML {
namespace {
TEST(NodeTest, CloneNull) {
  Node node;
  Node clone = Clone(node);
  EXPECT_EQ(NodeType::Null, clone.Type());
}
}
}
