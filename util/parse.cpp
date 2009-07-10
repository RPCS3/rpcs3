#include "yaml.h"
#include <fstream>
#include <iostream>

int main(int argc, char **argv)
{
	if(argc != 2) {
		std::cout << "Usage: " << argv[0] << " input-file\n";
		return 0;
	}

	std::ifstream fin(argv[1]);
	try {
		YAML::Parser parser(fin);
		YAML::Node doc;
		parser.GetNextDocument(doc);
	} catch(const YAML::Exception& e) {
		std::cerr << "Error at line " << e.line << ", col " << e.column << ": " << e.msg << "\n";
	}
	return 0;
}
