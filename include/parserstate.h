#pragma once

#ifndef PARSERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define PARSERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66


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

#endif // PARSERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
