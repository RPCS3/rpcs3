#include "yaml-cpp/value/parse.h"
#include "yaml-cpp/value/value.h"
#include "yaml-cpp/value/impl.h"
#include "yaml-cpp/parser.h"
#include "valuebuilder.h"

#include <sstream>

namespace YAML
{
	Value Parse(const std::string& input) {
		std::stringstream stream(input);
		return Parse(stream);
	}
	
	Value Parse(const char *input) {
		std::stringstream stream(input);
		return Parse(stream);
	}
	
	Value Parse(std::istream& input) {
		Parser parser(input);
		ValueBuilder builder;
		if(!parser.HandleNextDocument(builder))
			return Value();
		
		return builder.Root();
	}
}
