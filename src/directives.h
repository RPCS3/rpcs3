#pragma once

#ifndef DIRECTIVES_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define DIRECTIVES_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include <string>
#include <map>

namespace YAML
{
	struct Version {
		bool isDefault;
		int major, minor;
	};
	
	struct Directives {
		Directives();
		
		const std::string TranslateTagHandle(const std::string& handle) const;

		Version version;
		std::map<std::string, std::string> tags;
	};
}

#endif // DIRECTIVES_H_62B23520_7C8E_11DE_8A39_0800200C9A66
