#include "nodetests.h"
#include "yaml-cpp/yaml.h"
#include <boost/foreach.hpp>
#include <iostream>

namespace {
	struct TEST {
		TEST(): ok(false) {}
		TEST(bool ok_): ok(ok_) {}
		TEST(const char *error_): ok(false), error(error_) {}
        TEST(const std::string& error_): ok(false), error(error_) {}
		
		bool ok;
		std::string error;
	};
}

#define YAML_ASSERT(cond)\
    do {\
        if(!(cond))\
            return "  Assert failed: " #cond;\
    } while(false)

#define YAML_ASSERT_THROWS(cond, exc)\
    do {\
        try {\
            (cond);\
            return "  Expression did not throw: " #cond;\
        } catch(const exc&) {\
        } catch(const std::runtime_error& e) {\
            std::stringstream stream;\
            stream << "  Expression threw runtime error ther than " #exc ":\n    " #cond "\n    " << e.what();\
            return stream.str();\
        } catch(...) {\
            return "  Expression threw unknown exception, other than " #exc ":\n    " #cond;\
        }\
    } while(false)

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
			node.push_back(10);
			node.push_back("foo");
			node.push_back("monkey");
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
		
		TEST StdPair()
		{
			std::pair<int, std::string> p;
            p.first = 5;
            p.second = "five";
			
			YAML::Node node;
			node["pair"] = p;
			YAML_ASSERT((node["pair"].as<std::pair<int, std::string> >() == p));
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
        
        TEST FallbackValues()
        {
            YAML::Node node = YAML::Load("foo: bar\nx: 2");
            YAML_ASSERT(node["foo"].as<std::string>() == "bar");
            YAML_ASSERT(node["foo"].as<std::string>("hello") == "bar");
            YAML_ASSERT(node["baz"].as<std::string>("hello") == "hello");
            YAML_ASSERT(node["x"].as<int>() == 2);
            YAML_ASSERT(node["x"].as<int>(5) == 2);
            YAML_ASSERT(node["y"].as<int>(5) == 5);
            return true;
        }
        
        TEST NumericConversion()
        {
            YAML::Node node = YAML::Load("[1.5, 1, .nan, .inf, -.inf, 0x15, 015]");
            YAML_ASSERT(node[0].as<float>() == 1.5f);
            YAML_ASSERT(node[0].as<double>() == 1.5);
            YAML_ASSERT_THROWS(node[0].as<int>(), YAML::TypedBadConversion<int>);
            YAML_ASSERT(node[1].as<int>() == 1);
            YAML_ASSERT(node[1].as<float>() == 1.0f);
            YAML_ASSERT(node[2].as<float>() != node[2].as<float>());
            YAML_ASSERT(node[3].as<float>() == std::numeric_limits<float>::infinity());
            YAML_ASSERT(node[4].as<float>() == -std::numeric_limits<float>::infinity());
            YAML_ASSERT(node[5].as<int>() == 21);
            YAML_ASSERT(node[6].as<int>() == 13);
            return true;
        }
        
        TEST Binary()
        {
            YAML::Node node = YAML::Load("[!!binary \"SGVsbG8sIFdvcmxkIQ==\", !!binary \"TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4K\"]");
            YAML_ASSERT(node[0].as<YAML::Binary>() == YAML::Binary(reinterpret_cast<const unsigned char*>("Hello, World!"), 13));
            YAML_ASSERT(node[1].as<YAML::Binary>() == YAML::Binary(reinterpret_cast<const unsigned char*>("Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.\n"), 270));
            return true;
        }
        
        TEST IterateSequence()
        {
            YAML::Node node = YAML::Load("[1, 3, 5, 7]");
            int seq[] = {1, 3, 5, 7};
            int i=0;
            for(YAML::const_iterator it=node.begin();it!=node.end();++it) {
                YAML_ASSERT(i < 4);
                int x = seq[i++];
                YAML_ASSERT(it->as<int>() == x);
            }
            YAML_ASSERT(i == 4);
            return true;
        }
        
        TEST IterateMap()
        {
            YAML::Node node = YAML::Load("{a: A, b: B, c: C}");
            int i=0;
            for(YAML::const_iterator it=node.begin();it!=node.end();++it) {
                YAML_ASSERT(i < 3);
                i++;
                YAML_ASSERT(it->first.as<char>() + 'A' - 'a' == it->second.as<char>());
            }
            YAML_ASSERT(i == 3);
            return true;
        }
        
        TEST ForEach()
        {
            YAML::Node node = YAML::Load("[1, 3, 5, 7]");
            int seq[] = {1, 3, 5, 7};
            int i = 0;
            BOOST_FOREACH(const YAML::Node &item, node) {
                int x = seq[i++];
                YAML_ASSERT(item.as<int>() == x);
            }
            return true;
        }

        TEST ForEachMap()
        {
            YAML::Node node = YAML::Load("{a: A, b: B, c: C}");
            BOOST_FOREACH(const YAML::const_iterator::value_type &p, node) {
                YAML_ASSERT(p.first.as<char>() + 'A' - 'a' == p.second.as<char>());
            }
            return true;
        }
        
        TEST CloneScalar()
        {
            YAML::Node node = YAML::Load("!foo monkey");
            YAML::Node clone = Clone(node);
            YAML_ASSERT(!(node == clone));
            YAML_ASSERT(node.as<std::string>() == clone.as<std::string>());
            YAML_ASSERT(node.Tag() == clone.Tag());
            return true;
        }

        TEST CloneSeq()
        {
            YAML::Node node = YAML::Load("[1, 3, 5, 7]");
            YAML::Node clone = Clone(node);
            YAML_ASSERT(!(node == clone));
            YAML_ASSERT(clone.Type() == YAML::NodeType::Sequence);
            YAML_ASSERT(node.size() == clone.size());
            for(std::size_t i=0;i<node.size();i++)
                YAML_ASSERT(node[i].as<int>() == clone[i].as<int>());
            return true;
        }

        TEST CloneMap()
        {
            YAML::Node node = YAML::Load("{foo: bar}");
            YAML::Node clone = Clone(node);
            YAML_ASSERT(!(node == clone));
            YAML_ASSERT(clone.Type() == YAML::NodeType::Map);
            YAML_ASSERT(node.size() == clone.size());
            YAML_ASSERT(node["foo"].as<std::string>() == clone["foo"].as<std::string>());
            return true;
        }

        TEST CloneAlias()
        {
            YAML::Node node = YAML::Load("&foo [*foo]");
            YAML::Node clone = Clone(node);
            YAML_ASSERT(!(node == clone));
            YAML_ASSERT(clone.Type() == YAML::NodeType::Sequence);
            YAML_ASSERT(node.size() == clone.size());
            YAML_ASSERT(clone == clone[0]);
            return true;
        }
        
        TEST ForceInsertIntoMap()
        {
            YAML::Node node;
            node["a"] = "b";
            node.force_insert("x", "y");
            node.force_insert("a", 5);
            YAML_ASSERT(node.size() == 3);
            YAML_ASSERT(node.Type() == YAML::NodeType::Map);
            bool ab = false;
            bool a5 = false;
            bool xy = false;
            for(YAML::const_iterator it=node.begin();it!=node.end();++it) {
                if(it->first.as<std::string>() == "a") {
                    if(it->second.as<std::string>() == "b")
                        ab = true;
                    else if(it->second.as<std::string>() == "5")
                        a5 = true;
                } else if(it->first.as<std::string>() == "x" && it->second.as<std::string>() == "y")
                    xy = true;
            }
            YAML_ASSERT(ab);
            YAML_ASSERT(a5);
            YAML_ASSERT(xy);
            return true;
        }
        
        TEST ResetNode()
        {
            YAML::Node node = YAML::Load("[1, 2, 3]");
            YAML_ASSERT(!node.IsNull());
            YAML::Node other = node;
            node.reset();
            YAML_ASSERT(node.IsNull());
            YAML_ASSERT(!other.IsNull());
            node.reset(other);
            YAML_ASSERT(!node.IsNull());
            YAML_ASSERT(other == node);
            return true;
        }

        TEST DereferenceIteratorError()
        {
            YAML::Node node = YAML::Load("[{a: b}, 1, 2]");
            YAML_ASSERT_THROWS(node.begin()->first.as<int>(), YAML::InvalidNode);
            YAML_ASSERT((*node.begin()).IsMap() == true);
            YAML_ASSERT(node.begin()->IsMap() == true);
            YAML_ASSERT_THROWS((*node.begin()->begin()).IsDefined(), YAML::InvalidNode);
            YAML_ASSERT_THROWS(node.begin()->begin()->IsDefined(), YAML::InvalidNode);
            return true;
        }

        TEST FloatingPrecision()
        {
            const double x = 0.123456789;
            YAML::Node node = YAML::Node(x);
            YAML_ASSERT(node.as<double>() == x);
            return true;
        }
    }
	
	void RunNodeTest(TEST (*test)(), const std::string& name, int& passed, int& total) {
		TEST ret;
		try {
			ret = test();
		} catch(const std::exception& e) {
			ret.ok = false;
            ret.error = e.what();
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
		RunNodeTest(&Node::StdPair, "std::pair", passed, total);
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
		RunNodeTest(&Node::FallbackValues, "fallback values", passed, total);
		RunNodeTest(&Node::NumericConversion, "numeric conversion", passed, total);
		RunNodeTest(&Node::Binary, "binary", passed, total);
		RunNodeTest(&Node::IterateSequence, "iterate sequence", passed, total);
		RunNodeTest(&Node::IterateMap, "iterate map", passed, total);
		RunNodeTest(&Node::ForEach, "for each", passed, total);
		RunNodeTest(&Node::ForEachMap, "for each map", passed, total);
		RunNodeTest(&Node::CloneScalar, "clone scalar", passed, total);
		RunNodeTest(&Node::CloneSeq, "clone seq", passed, total);
		RunNodeTest(&Node::CloneMap, "clone map", passed, total);
		RunNodeTest(&Node::CloneAlias, "clone alias", passed, total);
        RunNodeTest(&Node::ForceInsertIntoMap, "force insert into map", passed, total);
        RunNodeTest(&Node::ResetNode, "reset node", passed, total);
        RunNodeTest(&Node::DereferenceIteratorError, "dereference iterator error", passed, total);
        RunNodeTest(&Node::FloatingPrecision, "floating precision", passed, total);

		std::cout << "Node tests: " << passed << "/" << total << " passed\n";
		return passed == total;
	}
}
