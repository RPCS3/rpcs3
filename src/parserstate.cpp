#include "parserstate.h"

namespace YAML
{
	ParserState::ParserState()
	{
		// version
		version.isDefault = true;
		version.major = 1;
		version.minor = 2;
	}

	const std::string ParserState::TranslateTagHandle(const std::string& handle) const
	{
		std::map <std::string, std::string>::const_iterator it = tags.find(handle);
		if(it == tags.end()) {
			if(handle == "!!")
				return "tag:yaml.org,2002:";
			return handle;
		}

		return it->second;
	}
}
