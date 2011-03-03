#include "yaml-cpp/yaml.h"
#include "yaml-cpp/eventhandler.h"
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

class NullEventHandler: public YAML::EventHandler
{
public:
	virtual void OnDocumentStart(const YAML::Mark&) {}
	virtual void OnDocumentEnd() {}
	
	virtual void OnNull(const YAML::Mark&, YAML::anchor_t) {}
	virtual void OnAlias(const YAML::Mark&, YAML::anchor_t) {}
	virtual void OnScalar(const YAML::Mark&, const std::string&, YAML::anchor_t, const std::string&) {}
	
	virtual void OnSequenceStart(const YAML::Mark&, const std::string&, YAML::anchor_t) {}
	virtual void OnSequenceEnd() {}
	
	virtual void OnMapStart(const YAML::Mark&, const std::string&, YAML::anchor_t) {}
	virtual void OnMapEnd() {}
};

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
		NullEventHandler handler;
		while(parser.GetNextDocument(doc)) {
			YAML::Emitter emitter;
			emitter << doc;
			std::cout << emitter.c_str() << "\n";
		}
	} catch(const YAML::Exception& e) {
		std::cerr << e.what() << "\n";
	}
	return 0;
}
