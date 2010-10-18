#include "yaml.h"
#include <fstream>
#include <iostream>
#include <vector>

struct Params {
	bool hasFile;
	std::string fileName;
};

Params ParseArgs(int argc, char **argv) {
	Params p;

	std::vector<std::string> args(argv + 1, argv + argc);
	
	return p;
}

int main(int argc, char **argv)
{
	Params p = ParseArgs(argc, argv);

	std::ifstream fin;
	if(argc > 1)
		fin.open(argv[1]);
	
	std::istream& input = (argc > 1 ? fin : std::cin);
	try {
		YAML::Parser parser(input);
		YAML::Node doc;
		while(parser.GetNextDocument(doc)) {
//			YAML::Emitter emitter;
//			emitter << doc;
//			std::cout << emitter.c_str() << "\n";
		}
	} catch(const YAML::Exception& e) {
		std::cerr << e.what() << "\n";
	}
	return 0;
}
