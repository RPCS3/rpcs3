#pragma once

#include <string>
#include <sstream>

namespace YAML
{
	template <typename T>
	struct Converter {
		static bool Convert(const std::string& input, T& output);
	};

	template <typename T>
	bool Convert(const std::string& input, T& output) {
		return Converter<T>::Convert(input, output);
	}
	
	// this is the one to specialize
	template <typename T>
	inline bool Converter<T>::Convert(const std::string& input, T& output) {
		std::stringstream stream(input);
		stream >> output;
		return !stream.fail();
	}

	// specializations
	template <>
	inline bool Converter<std::string>::Convert(const std::string& input, std::string& output) {
		output = input;
		return true;
	}

	template <>
	bool Converter<bool>::Convert(const std::string& input, bool& output);
}
