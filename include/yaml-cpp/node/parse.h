#ifndef VALUE_PARSE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_PARSE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <string>
#include <iosfwd>

namespace YAML
{
	class Value;
	
	Value Parse(const std::string& input);
	Value Parse(const char *input);
	Value Parse(std::istream& input);
}

#endif // VALUE_PARSE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

