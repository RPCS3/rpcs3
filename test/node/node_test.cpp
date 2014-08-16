#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/impl.h"
#include "yaml-cpp/node/convert.h"
#include "yaml-cpp/node/iterator.h"
#include "yaml-cpp/node/detail/impl.h"

#include "gtest/gtest.h"

namespace YAML {
namespace {
TEST(NodeTest, SimpleScalar) {
  YAML::Node node = YAML::Node("Hello, World!");
  EXPECT_TRUE(node.IsScalar());
  EXPECT_EQ("Hello, World!", node.as<std::string>());
}

TEST(NodeTest, IntScalar) {
  YAML::Node node = YAML::Node(15);
  EXPECT_TRUE(node.IsScalar());
  EXPECT_EQ(15, node.as<int>());
}

TEST(NodeTest, SimpleAppendSequence) {
  YAML::Node node;
  node.push_back(10);
  node.push_back("foo");
  node.push_back("monkey");
  EXPECT_TRUE(node.IsSequence());
  EXPECT_EQ(3, node.size());
  EXPECT_EQ(10, node[0].as<int>());
  EXPECT_EQ("foo", node[1].as<std::string>());
  EXPECT_EQ("monkey", node[2].as<std::string>());
  EXPECT_TRUE(node.IsSequence());
}

TEST(NodeTest, SimpleAssignSequence) {
  YAML::Node node;
  node[0] = 10;
  node[1] = "foo";
  node[2] = "monkey";
  EXPECT_TRUE(node.IsSequence());
  EXPECT_EQ(3, node.size());
  EXPECT_EQ(10, node[0].as<int>());
  EXPECT_EQ("foo", node[1].as<std::string>());
  EXPECT_EQ("monkey", node[2].as<std::string>());
  EXPECT_TRUE(node.IsSequence());
}

TEST(NodeTest, SimpleMap) {
  YAML::Node node;
  node["key"] = "value";
  EXPECT_TRUE(node.IsMap());
  EXPECT_EQ("value", node["key"].as<std::string>());
  EXPECT_EQ(1, node.size());
}

TEST(NodeTest, MapWithUndefinedValues) {
  YAML::Node node;
  node["key"] = "value";
  node["undefined"];
  EXPECT_TRUE(node.IsMap());
  EXPECT_EQ("value", node["key"].as<std::string>());
  EXPECT_EQ(1, node.size());

  node["undefined"] = "monkey";
  EXPECT_EQ("monkey", node["undefined"].as<std::string>());
  EXPECT_EQ(2, node.size());
}

TEST(NodeTest, MapIteratorWithUndefinedValues) {
  YAML::Node node;
  node["key"] = "value";
  node["undefined"];

  std::size_t count = 0;
  for (const_iterator it = node.begin(); it != node.end(); ++it)
    count++;
  EXPECT_EQ(1, count);
}

TEST(NodeTest, SimpleSubkeys) {
  YAML::Node node;
  node["device"]["udid"] = "12345";
  node["device"]["name"] = "iPhone";
  node["device"]["os"] = "4.0";
  node["username"] = "monkey";
  EXPECT_EQ("12345", node["device"]["udid"].as<std::string>());
  EXPECT_EQ("iPhone", node["device"]["name"].as<std::string>());
  EXPECT_EQ("4.0", node["device"]["os"].as<std::string>());
  EXPECT_EQ("monkey", node["username"].as<std::string>());
}

TEST(NodeTest, StdVector) {
  std::vector<int> primes;
  primes.push_back(2);
  primes.push_back(3);
  primes.push_back(5);
  primes.push_back(7);
  primes.push_back(11);
  primes.push_back(13);

  YAML::Node node;
  node["primes"] = primes;
  EXPECT_EQ(primes, node["primes"].as<std::vector<int> >());
}

TEST(NodeTest, StdList) {
  std::list<int> primes;
  primes.push_back(2);
  primes.push_back(3);
  primes.push_back(5);
  primes.push_back(7);
  primes.push_back(11);
  primes.push_back(13);

  YAML::Node node;
  node["primes"] = primes;
  EXPECT_EQ(primes, node["primes"].as<std::list<int> >());
}

TEST(NodeTest, StdMap) {
  std::map<int, int> squares;
  squares[0] = 0;
  squares[1] = 1;
  squares[2] = 4;
  squares[3] = 9;
  squares[4] = 16;

  YAML::Node node;
  node["squares"] = squares;
  std::map<int, int> actualSquares = node["squares"].as<std::map<int, int> >();
  EXPECT_EQ(squares, actualSquares);
}

TEST(NodeTest, StdPair) {
  std::pair<int, std::string> p;
  p.first = 5;
  p.second = "five";

  YAML::Node node;
  node["pair"] = p;
  std::pair<int, std::string> actualP =
      node["pair"].as<std::pair<int, std::string> >();
  EXPECT_EQ(p, actualP);
}

TEST(NodeTest, SimpleAlias) {
  YAML::Node node;
  node["foo"] = "value";
  node["bar"] = node["foo"];
  EXPECT_EQ("value", node["foo"].as<std::string>());
  EXPECT_EQ("value", node["bar"].as<std::string>());
  EXPECT_EQ(node["bar"], node["foo"]);
  EXPECT_EQ(2, node.size());
}

TEST(NodeTest, AliasAsKey) {
  YAML::Node node;
  node["foo"] = "value";
  YAML::Node value = node["foo"];
  node[value] = "foo";
  EXPECT_EQ("value", node["foo"].as<std::string>());
  EXPECT_EQ("foo", node[value].as<std::string>());
  EXPECT_EQ("foo", node["value"].as<std::string>());
  EXPECT_EQ(2, node.size());
}

TEST(NodeTest, SelfReferenceSequence) {
  YAML::Node node;
  node[0] = node;
  EXPECT_TRUE(node.IsSequence());
  EXPECT_EQ(1, node.size());
  EXPECT_EQ(node, node[0]);
  EXPECT_EQ(node, node[0][0]);
  EXPECT_EQ(node[0], node[0][0]);
}

TEST(NodeTest, ValueSelfReferenceMap) {
  YAML::Node node;
  node["key"] = node;
  EXPECT_TRUE(node.IsMap());
  EXPECT_EQ(1, node.size());
  EXPECT_EQ(node, node["key"]);
  EXPECT_EQ(node, node["key"]["key"]);
  EXPECT_EQ(node["key"], node["key"]["key"]);
}

TEST(NodeTest, KeySelfReferenceMap) {
  YAML::Node node;
  node[node] = "value";
  EXPECT_TRUE(node.IsMap());
  EXPECT_EQ(1, node.size());
  EXPECT_EQ("value", node[node].as<std::string>());
}

TEST(NodeTest, SelfReferenceMap) {
  YAML::Node node;
  node[node] = node;
  EXPECT_TRUE(node.IsMap());
  EXPECT_EQ(1, node.size());
  EXPECT_EQ(node, node[node]);
  EXPECT_EQ(node, node[node][node]);
  EXPECT_EQ(node[node], node[node][node]);
}

TEST(NodeTest, TempMapVariable) {
  YAML::Node node;
  YAML::Node tmp = node["key"];
  tmp = "value";
  EXPECT_TRUE(node.IsMap());
  EXPECT_EQ(1, node.size());
  EXPECT_EQ("value", node["key"].as<std::string>());
}

TEST(NodeTest, TempMapVariableAlias) {
  YAML::Node node;
  YAML::Node tmp = node["key"];
  tmp = node["other"];
  node["other"] = "value";
  EXPECT_TRUE(node.IsMap());
  EXPECT_EQ(2, node.size());
  EXPECT_EQ("value", node["key"].as<std::string>());
  EXPECT_EQ("value", node["other"].as<std::string>());
  EXPECT_EQ(node["key"], node["other"]);
}

TEST(NodeTest, Bool) {
  YAML::Node node;
  node[true] = false;
  EXPECT_TRUE(node.IsMap());
  EXPECT_EQ(false, node[true].as<bool>());
}

TEST(NodeTest, AutoBoolConversion) {
#ifdef _MSC_VER
#pragma warning(disable : 4800)
#endif
  YAML::Node node;
  node["foo"] = "bar";
  EXPECT_TRUE(static_cast<bool>(node["foo"]));
  EXPECT_TRUE(!node["monkey"]);
  EXPECT_TRUE(!!node["foo"]);
}

TEST(NodeTest, FloatingPrecision) {
  const double x = 0.123456789;
  YAML::Node node = YAML::Node(x);
  EXPECT_EQ(x, node.as<double>());
}

TEST(NodeTest, SpaceChar) {
  YAML::Node node = YAML::Node(' ');
  EXPECT_EQ(' ', node.as<char>());
}

TEST(NodeTest, CloneNull) {
  Node node;
  Node clone = Clone(node);
  EXPECT_EQ(NodeType::Null, clone.Type());
}

TEST(NodeTest, KeyNodeExitsScope) {
  Node node;
  {
    Node temp("Hello, world");
    node[temp] = 0;
  }
  for (const auto &kv : node) {
    (void)kv;
  }
}
}
}
