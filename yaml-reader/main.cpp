#include "yaml.h"
#include "tests.h"
#include <fstream>
#include <iostream>
#include <cstring>

#include "emitter.h"
#include "stlemitter.h"

void run()
{
	std::ifstream fin("tests/test.yaml");
	YAML::Parser parser(fin);

	while(parser)
	{
		YAML::Node doc;
		parser.GetNextDocument(doc);
		std::cout << doc;
	}
	
	// try some output
	YAML::Emitter out;
}

int main(int argc, char **argv)
{
	bool verbose = false;
	for(int i=1;i<argc;i++) {
		if(std::strcmp(argv[i], "-v") == 0)
			verbose = true;
	}

#ifdef WINDOWS
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
#endif // WINDOWS
	Test::RunAll(verbose);
	run();

	return 0;
}
