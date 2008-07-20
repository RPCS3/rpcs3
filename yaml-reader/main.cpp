#include "yaml.h"
#include "tests.h"
#include <fstream>
#include <iostream>

#ifdef _DEBUG
#pragma comment(lib, "yamlcppd.lib")
#else
#pragma comment(lib, "yamlcpp.lib")
#endif

void run()
{
	std::ifstream fin("yaml-reader/tests/test.yaml");

	try {
		YAML::Parser parser(fin);
		if(!parser)
			return;

		YAML::Node doc;
		parser.GetNextDocument(doc);
		std::cout << doc;
	} catch(YAML::Exception&) {
		std::cout << "Error parsing the yaml!\n";
	}
}

int main()
{	
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
	Test::RunAll();
	run();

	getchar();
	return 0;
}
