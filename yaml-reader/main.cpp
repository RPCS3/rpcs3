#include "yaml.h"
#include "tests.h"
#include <fstream>
#include <iostream>
#include <cstring>

void run()
{
	std::ifstream fin("tests/test.yaml");

	try {
		YAML::Parser parser(fin);
		YAML::Node doc;
		parser.GetNextDocument(doc);

		std::cout << "name: " << doc["name"] << "\n";
		std::cout << "age: " << doc["age"] << "\n";
	} catch(YAML::TypedKeyNotFound <std::string>& e) {
		std::cout << "Key '" << e.key << "' not found at line " << e.line+1 << ", col " << e.column+1 << "\n";
	} catch(YAML::KeyNotFound& e) {
		std::cout << "Key not found at line " << e.line+1 << ", col " << e.column+1 << "\n";
	} catch(YAML::Exception& e) {
		std::cout << "Error at line " << e.line+1 << ", col " << e.column+1 << ": " << e.msg << "\n";
	}
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
