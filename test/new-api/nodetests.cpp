#include "nodetests.h"
#include "yaml-cpp/yaml.h"
#include <iostream>

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
			YAML_ASSERT(node.IsScalar());
			YAML_ASSERT(node.as<std::string>() == "Hello, World!");
			return true;
		}
		
		TEST IntScalar()
		{
			YAML::Node node = YAML::Node(15);
			YAML_ASSERT(node.IsScalar());
			YAML_ASSERT(node.as<int>() == 15);
			return true;
		}

		TEST SimpleAppendSequence()
		{
			YAML::Node node;
			node.append(10);
			node.append("foo");
			node.append("monkey");
			YAML_ASSERT(node.IsSequence());
			YAML_ASSERT(node.size() == 3);
			YAML_ASSERT(node[0].as<int>() == 10);
			YAML_ASSERT(node[1].as<std::string>() == "foo");
			YAML_ASSERT(node[2].as<std::string>() == "monkey");
			YAML_ASSERT(node.IsSequence());
			return true;
		}

		TEST SimpleAssignSequence()
		{
			YAML::Node node;
			node[0] = 10;
			node[1] = "foo";
			node[2] = "monkey";
			YAML_ASSERT(node.IsSequence());
			YAML_ASSERT(node.size() == 3);
			YAML_ASSERT(node[0].as<int>() == 10);
			YAML_ASSERT(node[1].as<std::string>() == "foo");
			YAML_ASSERT(node[2].as<std::string>() == "monkey");
			YAML_ASSERT(node.IsSequence());
			return true;
		}

		TEST SimpleMap()
		{
			YAML::Node node;
			node["key"] = "value";
			YAML_ASSERT(node.IsMap());
			YAML_ASSERT(node["key"].as<std::string>() == "value");
			YAML_ASSERT(node.size() == 1);
			return true;
		}

		TEST MapWithUndefinedValues()
		{
			YAML::Node node;
			node["key"] = "value";
			node["undefined"];
			YAML_ASSERT(node.IsMap());
			YAML_ASSERT(node["key"].as<std::string>() == "value");
			YAML_ASSERT(node.size() == 1);

			node["undefined"] = "monkey";
			YAML_ASSERT(node["undefined"].as<std::string>() == "monkey");
			YAML_ASSERT(node.size() == 2);
			
			return true;
		}
		
		TEST MapIteratorWithUndefinedValues()
		{
			YAML::Node node;
			node["key"] = "value";
			node["undefined"];
			
			std::size_t count = 0;
			for(YAML::const_iterator it=node.begin();it!=node.end();++it)
				count++;
			YAML_ASSERT(count == 1);
			return true;
		}
		
		TEST SimpleSubkeys()
		{
			YAML::Node node;
			node["device"]["udid"] = "12345";
			node["device"]["name"] = "iPhone";
			node["device"]["os"] = "4.0";
			node["username"] = "monkey";
			YAML_ASSERT(node["device"]["udid"].as<std::string>() == "12345");
			YAML_ASSERT(node["device"]["name"].as<std::string>() == "iPhone");
			YAML_ASSERT(node["device"]["os"].as<std::string>() == "4.0");
			YAML_ASSERT(node["username"].as<std::string>() == "monkey");
			return true;
		}
		
		TEST StdVector()
		{
			std::vector<int> primes;
			primes.push_back(2);
			primes.push_back(3);
			primes.push_back(5);
			primes.push_back(7);
			primes.push_back(11);
			primes.push_back(13);
			
			YAML::Node node;
			node["primes"] = primes;
			YAML_ASSERT(node["primes"].as<std::vector<int> >() == primes);
			return true;
		}

		TEST StdList()
		{
			std::list<int> primes;
			primes.push_back(2);
			primes.push_back(3);
			primes.push_back(5);
			primes.push_back(7);
			primes.push_back(11);
			primes.push_back(13);
			
			YAML::Node node;
			node["primes"] = primes;
			YAML_ASSERT(node["primes"].as<std::list<int> >() == primes);
			return true;
		}

		TEST StdMap()
		{
			std::map<int, int> squares;
			squares[0] = 0;
			squares[1] = 1;
			squares[2] = 4;
			squares[3] = 9;
			squares[4] = 16;
			
			YAML::Node node;
			node["squares"] = squares;
			YAML_ASSERT((node["squares"].as<std::map<int, int> >() == squares));
			return true;
		}
		
		TEST SimpleAlias()
		{
			YAML::Node node;
			node["foo"] = "value";
			node["bar"] = node["foo"];
			YAML_ASSERT(node["foo"].as<std::string>() == "value");
			YAML_ASSERT(node["bar"].as<std::string>() == "value");
			YAML_ASSERT(node["foo"] == node["bar"]);
			YAML_ASSERT(node.size() == 2);
			return true;
		}

		TEST AliasAsKey()
		{
			YAML::Node node;
			node["foo"] = "value";
			YAML::Node value = node["foo"];
			node[value] = "foo";
			YAML_ASSERT(node["foo"].as<std::string>() == "value");
			YAML_ASSERT(node[value].as<std::string>() == "foo");
			YAML_ASSERT(node["value"].as<std::string>() == "foo");
			YAML_ASSERT(node.size() == 2);
			return true;
		}
		
		TEST SelfReferenceSequence()
		{
			YAML::Node node;
			node[0] = node;
			YAML_ASSERT(node.IsSequence());
			YAML_ASSERT(node.size() == 1);
			YAML_ASSERT(node[0] == node);
			YAML_ASSERT(node[0][0] == node);
			YAML_ASSERT(node[0][0] == node[0]);
			return true;
		}

		TEST ValueSelfReferenceMap()
		{
			YAML::Node node;
			node["key"] = node;
			YAML_ASSERT(node.IsMap());
			YAML_ASSERT(node.size() == 1);
			YAML_ASSERT(node["key"] == node);
			YAML_ASSERT(node["key"]["key"] == node);
			YAML_ASSERT(node["key"]["key"] == node["key"]);
			return true;
		}

		TEST KeySelfReferenceMap()
		{
			YAML::Node node;
			node[node] = "value";
			YAML_ASSERT(node.IsMap());
			YAML_ASSERT(node.size() == 1);
			YAML_ASSERT(node[node].as<std::string>() == "value");
			return true;
		}

		TEST SelfReferenceMap()
		{
			YAML::Node node;
			node[node] = node;
			YAML_ASSERT(node.IsMap());
			YAML_ASSERT(node.size() == 1);
			YAML_ASSERT(node[node] == node);
			YAML_ASSERT(node[node][node] == node);
			YAML_ASSERT(node[node][node] == node[node]);
			return true;
		}
		
		TEST TempMapVariable()
		{
			YAML::Node node;
			YAML::Node tmp = node["key"];
			tmp = "value";
			YAML_ASSERT(node.IsMap());
			YAML_ASSERT(node.size() == 1);
			YAML_ASSERT(node["key"].as<std::string>() == "value");
			return true;
		}

		TEST TempMapVariableAlias()
		{
			YAML::Node node;
			YAML::Node tmp = node["key"];
			tmp = node["other"];
			node["other"] = "value";
			YAML_ASSERT(node.IsMap());
			YAML_ASSERT(node.size() == 2);
			YAML_ASSERT(node["key"].as<std::string>() == "value");
			YAML_ASSERT(node["other"].as<std::string>() == "value");
			YAML_ASSERT(node["other"] == node["key"]);
			return true;
		}
		
		TEST Bool()
		{
			YAML::Node node;
			node[true] = false;
			YAML_ASSERT(node.IsMap());
			YAML_ASSERT(node[true].as<bool>() == false);
			return true;
		}
		
		TEST AutoBoolConversion()
		{
			YAML::Node node;
			node["foo"] = "bar";
			YAML_ASSERT(static_cast<bool>(node["foo"]));
			YAML_ASSERT(!node["monkey"]);
			YAML_ASSERT(!!node["foo"]);
			return true;
		}
        
        TEST Reassign()
        {
            YAML::Node node = YAML::Load("foo");
            node = YAML::Node();
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
		RunNodeTest(&Node::SimpleAppendSequence, "simple append sequence", passed, total);
		RunNodeTest(&Node::SimpleAssignSequence, "simple assign sequence", passed, total);
		RunNodeTest(&Node::SimpleMap, "simple map", passed, total);
		RunNodeTest(&Node::MapWithUndefinedValues, "map with undefined values", passed, total);
		RunNodeTest(&Node::MapIteratorWithUndefinedValues, "map iterator with undefined values", passed, total);
		RunNodeTest(&Node::SimpleSubkeys, "simple subkey", passed, total);
		RunNodeTest(&Node::StdVector, "std::vector", passed, total);
		RunNodeTest(&Node::StdList, "std::list", passed, total);
		RunNodeTest(&Node::StdMap, "std::map", passed, total);
		RunNodeTest(&Node::SimpleAlias, "simple alias", passed, total);
		RunNodeTest(&Node::AliasAsKey, "alias as key", passed, total);
		RunNodeTest(&Node::SelfReferenceSequence, "self reference sequence", passed, total);
		RunNodeTest(&Node::ValueSelfReferenceMap, "value self reference map", passed, total);
		RunNodeTest(&Node::KeySelfReferenceMap, "key self reference map", passed, total);
		RunNodeTest(&Node::SelfReferenceMap, "self reference map", passed, total);
		RunNodeTest(&Node::TempMapVariable, "temp map variable", passed, total);
		RunNodeTest(&Node::TempMapVariableAlias, "temp map variable alias", passed, total);
		RunNodeTest(&Node::Bool, "bool", passed, total);
		RunNodeTest(&Node::AutoBoolConversion, "auto bool conversion", passed, total);
		RunNodeTest(&Node::Reassign, "reassign", passed, total);

		std::cout << "Node tests: " << passed << "/" << total << " passed\n";
		return passed == total;
	}
}
