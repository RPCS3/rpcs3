#include "yaml-cpp/value.h"
#include <sstream>

namespace YAML
{
	// std::string
	template<>
	Value convert(const std::string& rhs) {
		return Value(rhs);
	}
	
	template<>
	bool convert(const Value& value, std::string& rhs) {
		if(value.Type() != ValueType::Scalar)
			return false;
		rhs = value.scalar();
		return true;
	}

#define YAML_DEFINE_CONVERT_STREAMABLE(type)\
	template<> Value convert(const type& rhs) {\
		std::stringstream stream;\
		stream << rhs;\
		return Value(stream.str());\
	}\
	template<> bool convert(const Value& value, type& rhs) {\
		if(value.Type() != ValueType::Scalar)\
			return false;\
		std::stringstream stream(value.scalar());\
		stream >> rhs;\
		return !!stream;\
	}

	YAML_DEFINE_CONVERT_STREAMABLE(int)
	YAML_DEFINE_CONVERT_STREAMABLE(unsigned)
	YAML_DEFINE_CONVERT_STREAMABLE(short)
	YAML_DEFINE_CONVERT_STREAMABLE(unsigned short)
	YAML_DEFINE_CONVERT_STREAMABLE(long)
	YAML_DEFINE_CONVERT_STREAMABLE(unsigned long)
	YAML_DEFINE_CONVERT_STREAMABLE(long long)
	YAML_DEFINE_CONVERT_STREAMABLE(unsigned long long)

	YAML_DEFINE_CONVERT_STREAMABLE(char)
	YAML_DEFINE_CONVERT_STREAMABLE(unsigned char)

	YAML_DEFINE_CONVERT_STREAMABLE(float)
	YAML_DEFINE_CONVERT_STREAMABLE(double)
	YAML_DEFINE_CONVERT_STREAMABLE(long double)
	
#undef YAML_DEFINE_CONVERT_STREAMABLE
}
