#include "yaml-cpp/value.h"

namespace YAML
{
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
}
