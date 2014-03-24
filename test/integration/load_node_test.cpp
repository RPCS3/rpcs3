#include "yaml-cpp/yaml.h"  // IWYU pragma: keep

#include "gtest/gtest.h"

namespace YAML {
namespace {
TEST(LoadNodeTest, Reassign) {
  YAML::Node node = YAML::Load("foo");
  node = YAML::Node();
}

TEST(LoadNodeTest, FallbackValues) {
  YAML::Node node = YAML::Load("foo: bar\nx: 2");
  EXPECT_EQ("bar", node["foo"].as<std::string>());
  EXPECT_EQ("bar", node["foo"].as<std::string>("hello"));
  EXPECT_EQ("hello", node["baz"].as<std::string>("hello"));
  EXPECT_EQ(2, node["x"].as<int>());
  EXPECT_EQ(2, node["x"].as<int>(5));
  EXPECT_EQ(5, node["y"].as<int>(5));
}

TEST(LoadNodeTest, NumericConversion) {
  YAML::Node node = YAML::Load("[1.5, 1, .nan, .inf, -.inf, 0x15, 015]");
  EXPECT_EQ(1.5f, node[0].as<float>());
  EXPECT_EQ(1.5, node[0].as<double>());
  EXPECT_THROW(node[0].as<int>(), YAML::TypedBadConversion<int>);
  EXPECT_EQ(1, node[1].as<int>());
  EXPECT_EQ(1.0f, node[1].as<float>());
  EXPECT_NE(node[2].as<float>(), node[2].as<float>());
  EXPECT_EQ(std::numeric_limits<float>::infinity(), node[3].as<float>());
  EXPECT_EQ(-std::numeric_limits<float>::infinity(), node[4].as<float>());
  EXPECT_EQ(21, node[5].as<int>());
  EXPECT_EQ(13, node[6].as<int>());
}

TEST(LoadNodeTest, Binary) {
  YAML::Node node = YAML::Load(
      "[!!binary \"SGVsbG8sIFdvcmxkIQ==\", !!binary "
      "\"TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieS"
      "B0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIG"
      "x1c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbi"
      "B0aGUgY29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZG"
      "dlLCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS"
      "4K\"]");
  EXPECT_EQ(
      YAML::Binary(reinterpret_cast<const unsigned char*>("Hello, World!"), 13),
      node[0].as<YAML::Binary>());
  EXPECT_EQ(YAML::Binary(reinterpret_cast<const unsigned char*>(
                             "Man is distinguished, not only by his reason, "
                             "but by this singular passion from other "
                             "animals, which is a lust of the mind, that by "
                             "a perseverance of delight in the continued and "
                             "indefatigable generation of knowledge, exceeds "
                             "the short vehemence of any carnal pleasure.\n"),
                         270),
            node[1].as<YAML::Binary>());
}

TEST(LoadNodeTest, IterateSequence) {
  YAML::Node node = YAML::Load("[1, 3, 5, 7]");
  int seq[] = {1, 3, 5, 7};
  int i = 0;
  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    EXPECT_TRUE(i < 4);
    int x = seq[i++];
    EXPECT_EQ(x, it->as<int>());
  }
  EXPECT_EQ(4, i);
}

TEST(LoadNodeTest, IterateMap) {
  YAML::Node node = YAML::Load("{a: A, b: B, c: C}");
  int i = 0;
  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    EXPECT_TRUE(i < 3);
    i++;
    EXPECT_EQ(it->second.as<char>(), it->first.as<char>() + 'A' - 'a');
  }
  EXPECT_EQ(3, i);
}

#ifdef BOOST_FOREACH
TEST(LoadNodeTest, ForEach) {
  YAML::Node node = YAML::Load("[1, 3, 5, 7]");
  int seq[] = {1, 3, 5, 7};
  int i = 0;
  BOOST_FOREACH(const YAML::Node & item, node) {
    int x = seq[i++];
    EXPECT_EQ(x, item.as<int>());
  }
}

TEST(LoadNodeTest, ForEachMap) {
  YAML::Node node = YAML::Load("{a: A, b: B, c: C}");
  BOOST_FOREACH(const YAML::const_iterator::value_type & p, node) {
    EXPECT_EQ(p.second.as<char>(), p.first.as<char>() + 'A' - 'a');
  }
}
#endif

TEST(LoadNodeTest, CloneScalar) {
  YAML::Node node = YAML::Load("!foo monkey");
  YAML::Node clone = Clone(node);
  EXPECT_FALSE(clone == node);
  EXPECT_EQ(clone.as<std::string>(), node.as<std::string>());
  EXPECT_EQ(clone.Tag(), node.Tag());
}

TEST(LoadNodeTest, CloneSeq) {
  YAML::Node node = YAML::Load("[1, 3, 5, 7]");
  YAML::Node clone = Clone(node);
  EXPECT_FALSE(clone == node);
  EXPECT_EQ(YAML::NodeType::Sequence, clone.Type());
  EXPECT_EQ(clone.size(), node.size());
  for (std::size_t i = 0; i < node.size(); i++) {
    EXPECT_EQ(clone[i].as<int>(), node[i].as<int>());
  }
}

TEST(LoadNodeTest, CloneMap) {
  YAML::Node node = YAML::Load("{foo: bar}");
  YAML::Node clone = Clone(node);
  EXPECT_FALSE(clone == node);
  EXPECT_EQ(YAML::NodeType::Map, clone.Type());
  EXPECT_EQ(clone.size(), node.size());
  EXPECT_EQ(clone["foo"].as<std::string>(), node["foo"].as<std::string>());
}

TEST(LoadNodeTest, CloneAlias) {
  YAML::Node node = YAML::Load("&foo [*foo]");
  YAML::Node clone = Clone(node);
  EXPECT_FALSE(clone == node);
  EXPECT_EQ(YAML::NodeType::Sequence, clone.Type());
  EXPECT_EQ(clone.size(), node.size());
  EXPECT_EQ(clone[0], clone);
}

TEST(LoadNodeTest, ForceInsertIntoMap) {
  YAML::Node node;
  node["a"] = "b";
  node.force_insert("x", "y");
  node.force_insert("a", 5);
  EXPECT_EQ(3, node.size());
  EXPECT_EQ(YAML::NodeType::Map, node.Type());
  bool ab = false;
  bool a5 = false;
  bool xy = false;
  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    if (it->first.as<std::string>() == "a") {
      if (it->second.as<std::string>() == "b")
        ab = true;
      else if (it->second.as<std::string>() == "5")
        a5 = true;
    } else if (it->first.as<std::string>() == "x" &&
               it->second.as<std::string>() == "y")
      xy = true;
  }
  EXPECT_TRUE(ab);
  EXPECT_TRUE(a5);
  EXPECT_TRUE(xy);
}

TEST(LoadNodeTest, ResetNode) {
  YAML::Node node = YAML::Load("[1, 2, 3]");
  EXPECT_TRUE(!node.IsNull());
  YAML::Node other = node;
  node.reset();
  EXPECT_TRUE(node.IsNull());
  EXPECT_TRUE(!other.IsNull());
  node.reset(other);
  EXPECT_TRUE(!node.IsNull());
  EXPECT_EQ(node, other);
}

TEST(LoadNodeTest, DereferenceIteratorError) {
  YAML::Node node = YAML::Load("[{a: b}, 1, 2]");
  EXPECT_THROW(node.begin()->first.as<int>(), YAML::InvalidNode);
  EXPECT_EQ(true, (*node.begin()).IsMap());
  EXPECT_EQ(true, node.begin()->IsMap());
  EXPECT_THROW((*node.begin()->begin()).IsDefined(), YAML::InvalidNode);
  EXPECT_THROW(node.begin()->begin()->IsDefined(), YAML::InvalidNode);
}

TEST(NodeTest, EmitEmptyNode) {
  YAML::Node node;
  YAML::Emitter emitter;
  emitter << node;
  EXPECT_EQ("", std::string(emitter.c_str()));
}
}
}
