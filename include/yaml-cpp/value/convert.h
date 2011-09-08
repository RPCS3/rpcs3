#ifndef VALUE_CONVERT_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_CONVERT_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/value/value.h"
#include <sstream>

namespace YAML
{
	// std::string
	template<>
	struct convert<std::string> {
		static Value encode(const std::string& rhs) {
			return Value(rhs);
		}
		
		static bool decode(const Value& value, std::string& rhs) {
			if(value.Type() != ValueType::Scalar)
				return false;
			rhs = value.scalar();
			return true;
		}
	};
}

#endif // VALUE_CONVERT_H_62B23520_7C8E_11DE_8A39_0800200C9A66
