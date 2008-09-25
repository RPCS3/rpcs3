#include "yaml.h"
#include "tests.h"
#include <fstream>
#include <iostream>

void run()
{
	std::ifstream fin("tests/test.yaml");

	try {
		YAML::Parser parser(fin);
		YAML::Node doc;
		parser.GetNextDocument(doc);
		for(unsigned i=0;i<doc.size();i++) {
			bool value;
			doc[i] >> value;
			std::cout << (value ? "true" : "false") << "\n";
		}
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
