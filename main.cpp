#include "parser.h"
#include <fstream>
#include <iostream>

int main()
{
	std::ifstream fin("test.yaml");
	YAML::Parser parser(fin);

	YAML::Document doc;
	while(parser) {
		std::cout << "---\n";
		parser.GetNextDocument(doc);
		std::cout << doc;
	}
	getchar();

	return 0;
}