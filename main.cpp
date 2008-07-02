#include "parser.h"
#include "node.h"
#include "exceptions.h"
#include <fstream>
#include <iostream>

int main()
{
	std::ifstream fin("test.yaml");

	try {
		YAML::Parser parser(fin);
		if(!parser)
			return 0;

		YAML::Document doc;
		parser.GetNextDocument(doc);

		const YAML::Node& root = doc.GetRoot();
		for(YAML::Node::Iterator it=root.begin();it!=root.end();++it) {
			std::cout << "Sequence:";
			for(YAML::Node::Iterator jt=it->begin();jt!=it->end();++jt) {
				int value;
				*jt >> value;
				std::cout << " " << value;
			}
			std::cout << std::endl;
		}
	} catch(YAML::Exception& e) {
		std::cout << "Error parsing the yaml!\n";
	}

	getchar();

	return 0;
}