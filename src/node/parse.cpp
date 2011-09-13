#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/impl.h"
#include "yaml-cpp/parser.h"
#include "nodebuilder.h"

#include <sstream>

namespace YAML
{
	Node Load(const std::string& input) {
		std::stringstream stream(input);
		return Load(stream);
	}
	
	Node Load(const char *input) {
		std::stringstream stream(input);
		return Load(stream);
	}
	
	Node Load(std::istream& input) {
		Parser parser(input);
		NodeBuilder builder;
		if(!parser.HandleNextDocument(builder))
			return Node();
		
		return builder.Root();
	}

	std::vector<Node> LoadAll(const std::string& input) {
		std::stringstream stream(input);
		return LoadAll(stream);
	}
	
	std::vector<Node> LoadAll(const char *input) {
		std::stringstream stream(input);
		return LoadAll(stream);
	}
	
	std::vector<Node> LoadAll(std::istream& input) {
		std::vector<Node> docs;
		
		Parser parser(input);
		while(1) {
			NodeBuilder builder;
			if(!parser.HandleNextDocument(builder))
				break;
			docs.push_back(builder.Root());
		}
		
		return docs;
	}
}
