#pragma once

#ifndef CONVERSION_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define CONVERSION_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "null.h"
#include "traits.h"
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
	
	template <typename T> 
	inline bool Convert(const std::string& input, T& output, typename enable_if<is_numeric<T> >::type * = 0) {
		std::stringstream stream(input);
		stream.unsetf(std::ios::dec);
		stream >> output;
		return !!stream;
	}
}

#endif // CONVERSION_H_62B23520_7C8E_11DE_8A39_0800200C9A66
