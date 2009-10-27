#include "parserstate.h"

namespace YAML
{
	void ParserState::Reset()
	{
		// version
		version.major = 1;
		version.minor = 2;

		// and tags
		tags.clear();
		tags["!"] = "!";
		tags["!!"] = "tag:yaml.org,2002:";
	}

	std::string ParserState::TranslateTag(const std::string& handle) const
	{
		std::map <std::string, std::string>::const_iterator it = tags.find(handle);
		if(it == tags.end())
			return handle;

		return it->second;
	}
}
