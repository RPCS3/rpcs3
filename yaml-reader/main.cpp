#include "yaml.h"
#include "tests.h"
#include <fstream>
#include <iostream>

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "yamlcppd.lib")
#else
#pragma comment(lib, "yamlcpp.lib")
#endif  // _DEBUG
#endif  // _MSC_VER

void run()
{
	std::ifstream fin("tests/test.yaml");

	try {
		YAML::Parser parser(fin);
		parser.PrintTokens(std::cout);
	} catch(YAML::Exception&) {
		std::cout << "Error parsing the yaml!\n";
	}
}

int main(int argc, char **argv)
{
	bool verbose = false;
	for(int i=1;i<argc;i++) {
		if(strcmp(argv[i], "-v") == 0)
			verbose = true;
	}

#ifdef WINDOWS
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
#endif // WINDOWS
	Test::RunAll(verbose);
	run();

	return 0;
}
