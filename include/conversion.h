#pragma once

#ifndef CONVERSION_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define CONVERSION_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "null.h"
#include <string>
#include <sstream>

namespace YAML
{
	inline bool Convert(const std::string& input, std::string& output) {
		output = input;
		return true;
	}
	
	bool Convert(const std::string& input, bool& output);
	bool Convert(const std::string& input, _Null& output);
	
#define YAML_MAKE_STREAM_CONVERT(type) \
	inline bool Convert(const std::string& input, type& output) { \
		std::stringstream stream(input); \
		stream >> output; \
		return !stream.fail(); \
	}
	
	YAML_MAKE_STREAM_CONVERT(char)
	YAML_MAKE_STREAM_CONVERT(unsigned char)
	YAML_MAKE_STREAM_CONVERT(int)
	YAML_MAKE_STREAM_CONVERT(unsigned int)
	YAML_MAKE_STREAM_CONVERT(short)
	YAML_MAKE_STREAM_CONVERT(unsigned short)
	YAML_MAKE_STREAM_CONVERT(long)
	YAML_MAKE_STREAM_CONVERT(unsigned long)
	YAML_MAKE_STREAM_CONVERT(float)
	YAML_MAKE_STREAM_CONVERT(double)
	YAML_MAKE_STREAM_CONVERT(long double)
	
#undef YAML_MAKE_STREAM_CONVERT
}

#endif // CONVERSION_H_62B23520_7C8E_11DE_8A39_0800200C9A66
