#include "nodetests.h"
#include "yaml-cpp/yaml.h"

namespace {
	struct TEST {
		TEST(): ok(false) {}
		TEST(bool ok_): ok(ok_) {}
		TEST(const char *error_): ok(false), error(error_) {}
		
		bool ok;
		std::string error;
	};
}

#define YAML_ASSERT(cond) do { if(!(cond)) return "  Assert failed: " #cond; } while(false)

namespace Test
{
	namespace Node
	{
		TEST SimpleScalar()
		{
			YAML::Node node = YAML::Node("Hello, World!");
			YAML_ASSERT(node.Type() == YAML::NodeType::Scalar);
			YAML_ASSERT(node.as<std::string>() == "Hello, World!");
			return true;
		}
		
		TEST IntScalar()
		{
			YAML::Node node = YAML::Node(15);
			YAML_ASSERT(node.Type() == YAML::NodeType::Scalar);
			YAML_ASSERT(node.as<int>() == 15);
			return true;
		}

		TEST SimpleSequence()
		{
			YAML::Node node;
			node.append(10);
			node.append("foo");
			node.append("monkey");
			YAML_ASSERT(node.Type() == YAML::NodeType::Sequence);
			YAML_ASSERT(node.size() == 3);
			YAML_ASSERT(node[0].as<int>() == 10);
			YAML_ASSERT(node[1].as<std::string>() == "foo");
			YAML_ASSERT(node[1].as<std::string>() == "monkey");
			return true;
		}

		TEST SimpleMap()
		{
			YAML::Node node;
			node["key"] = "value";
			YAML_ASSERT(node.Type() == YAML::NodeType::Map);
			YAML_ASSERT(node["key"].as<std::string>() == "value");
			YAML_ASSERT(node.size() == 1);
			return true;
		}

		TEST MapWithUndefinedValues()
		{
			YAML::Node node;
			node["key"] = "value";
			node["undefined"];
			YAML_ASSERT(node.Type() == YAML::NodeType::Map);
			YAML_ASSERT(node["key"].as<std::string>() == "value");
			YAML_ASSERT(node.size() == 1);
			return true;
		}
	}
	
	void RunNodeTest(TEST (*test)(), const std::string& name, int& passed, int& total) {
		TEST ret;
		try {
			ret = test();
		} catch(...) {
			ret.ok = false;
		}
		if(ret.ok) {
			passed++;
		} else {
			std::cout << "Node test failed: " << name << "\n";
			if(ret.error != "")
				std::cout << ret.error << "\n";
		}
		total++;
	}
	
	bool RunNodeTests()
	{
		int passed = 0;
		int total = 0;

		RunNodeTest(&Node::SimpleScalar, "simple scalar", passed, total);
		RunNodeTest(&Node::IntScalar, "int scalar", passed, total);
		RunNodeTest(&Node::SimpleSequence, "simple sequence", passed, total);
		RunNodeTest(&Node::SimpleMap, "simple map", passed, total);
		RunNodeTest(&Node::MapWithUndefinedValues, "map with undefined values", passed, total);

		std::cout << "Node tests: " << passed << "/" << total << " passed\n";
		return passed == total;
	}
}
