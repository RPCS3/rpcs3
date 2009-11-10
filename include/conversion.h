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
	
	template <typename>
	struct is_numeric { enum { value = false }; };

	template <> struct is_numeric <char> { enum { value = true }; };
	template <> struct is_numeric <unsigned char> { enum { value = true }; };
	template <> struct is_numeric <int> { enum { value = true }; };
	template <> struct is_numeric <unsigned int> { enum { value = true }; };
	template <> struct is_numeric <long int> { enum { value = true }; };
	template <> struct is_numeric <unsigned long int> { enum { value = true }; };
	template <> struct is_numeric <short int> { enum { value = true }; };
	template <> struct is_numeric <unsigned short int> { enum { value = true }; };
	template <> struct is_numeric <long long> { enum { value = true }; };
	template <> struct is_numeric <unsigned long long> { enum { value = true }; };
	template <> struct is_numeric <float> { enum { value = true }; };
	template <> struct is_numeric <double> { enum { value = true }; };
	template <> struct is_numeric <long double> { enum { value = true }; };


	template <typename T, typename, bool = T::value>
	struct enable_if;

	template <typename T, typename R>
	struct enable_if <T, R, true> {
		typedef R type;
	};

	template <typename T> 
	inline bool Convert(const std::string& input, T& output, typename enable_if<is_numeric<T>, T>::type * = 0) {
		std::stringstream stream(input);
		stream.unsetf(std::ios::dec);
		return stream >> output;
	}
}

#endif // CONVERSION_H_62B23520_7C8E_11DE_8A39_0800200C9A66
