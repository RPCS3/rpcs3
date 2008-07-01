#pragma once

#include <string>
#include <map>

namespace YAML
{
	struct Version {
		int major, minor;
	};

	struct ParserState
	{
		Version version;
		std::map <std::string, std::string> tags;

		void Reset();
		std::string TranslateTag(const std::string& handle) const;
	};
}
