#include "tests.h"
#include "emittertests.h"
#include "nodetests.h"
#include "parsertests.h"
#include "spectests.h"
#include "yaml-cpp/yaml.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

namespace Test
{
	void RunAll()
	{
		bool passed = true;
		if(!RunParserTests())
			passed = false;
			
		if(!RunEmitterTests())
			passed = false;

		if(!RunSpecTests())
			passed = false;

#ifndef YAML_CPP_OLD_API
		if(!RunNodeTests())
			passed = false;
#endif
		
		if(passed)
			std::cout << "All tests passed!\n";
	}
}

