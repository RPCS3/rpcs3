#include "parsertests.h"
#include "handlermacros.h"
#include "yaml-cpp/yaml.h"
#include <iostream>

namespace Test
{
    namespace Parser {
		TEST BadDocStart()
        {
            try {
                HANDLE("---{header: {id: 1");
            } catch(const YAML::ParserException& e) {
                YAML_ASSERT(true);
                return true;
            }
            return "  no exception caught";
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
				std::cout << "Spec test " << index << " failed: " << name << "\n";
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
		RunParserTest(&Parser::BadDocStart, "Bad doc start", passed, total);
		
		std::cout << "Parser tests: " << passed << "/" << total << " passed\n";
		return passed == total;
	}
}
