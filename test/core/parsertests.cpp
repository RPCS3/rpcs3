#include "parsertests.h"
#include "handlermacros.h"
#include "yaml-cpp/yaml.h"
#include <iostream>

namespace Test
{
    namespace Parser {
		TEST NoEndOfMapFlow()
        {
            try {
                HANDLE("---{header: {id: 1");
            } catch(const YAML::ParserException& e) {
                YAML_ASSERT(e.msg == std::string(YAML::ErrorMsg::END_OF_MAP_FLOW));
                return true;
            }
            return "  no exception caught";
        }
        
        TEST PlainScalarStartingWithQuestionMark()
        {
            HANDLE("foo: ?bar");
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "foo");
            EXPECT_SCALAR("?", 0, "?bar");
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }

        TEST NullStringScalar()
        {
            HANDLE("foo: null");
            EXPECT_DOC_START();
            EXPECT_MAP_START("?", 0);
            EXPECT_SCALAR("?", 0, "foo");
            EXPECT_NULL(0);
            EXPECT_MAP_END();
            EXPECT_DOC_END();
            DONE();
        }
    }
    
	namespace {
		void RunParserTest(TEST (*test)(), const std::string& name, int& passed, int& total) {
			TEST ret;
			try {
				ret = test();
			} catch(const YAML::Exception& e) {
				ret.ok = false;
				ret.error = std::string("  Exception caught: ") + e.what();
			}
			
			if(!ret.ok) {
				std::cout << "Parser test failed: " << name << "\n";
				std::cout << ret.error << "\n";
			}
			
			if(ret.ok)
				passed++;
			total++;
		}
	}
	
	bool RunParserTests()
	{
		int passed = 0;
		int total = 0;
		RunParserTest(&Parser::NoEndOfMapFlow, "No end of map flow", passed, total);
		RunParserTest(&Parser::PlainScalarStartingWithQuestionMark, "Plain scalar starting with question mark", passed, total);
		RunParserTest(&Parser::NullStringScalar, "Null string scalar", passed, total);
		
		std::cout << "Parser tests: " << passed << "/" << total << " passed\n";
		return passed == total;
	}
}
