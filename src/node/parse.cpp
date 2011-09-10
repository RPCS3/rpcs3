#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/impl.h"
#include "yaml-cpp/parser.h"
#include "nodebuilder.h"

#include <sstream>

namespace YAML
{
	Node Parse(const std::string& input) {
		std::stringstream stream(input);
		return Parse(stream);
	}
	
	Node Parse(const char *input) {
		std::stringstream stream(input);
		return Parse(stream);
	}
	
	Node Parse(std::istream& input) {
		Parser parser(input);
		NodeBuilder builder;
		if(!parser.HandleNextDocument(builder))
			return Node();
		
		return builder.Root();
	}
}
