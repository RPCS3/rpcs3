#ifndef VALUE_CONVERT_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_CONVERT_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/value/value.h"
#include <map>
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
	
#define YAML_DEFINE_CONVERT_STREAMABLE(type)\
	template<>\
	struct convert<type> {\
		static Value encode(const type& rhs) {\
			std::stringstream stream;\
			stream << rhs;\
			return Value(stream.str());\
		}\
		\
		static bool decode(const Value& value, type& rhs) {\
			if(value.Type() != ValueType::Scalar)\
				return false;\
			std::stringstream stream(value.scalar());\
			stream >> rhs;\
			return !!stream;\
		}\
	}
	
	YAML_DEFINE_CONVERT_STREAMABLE(int);
	YAML_DEFINE_CONVERT_STREAMABLE(unsigned);
	YAML_DEFINE_CONVERT_STREAMABLE(short);
	YAML_DEFINE_CONVERT_STREAMABLE(unsigned short);
	YAML_DEFINE_CONVERT_STREAMABLE(long);
	YAML_DEFINE_CONVERT_STREAMABLE(unsigned long);
	YAML_DEFINE_CONVERT_STREAMABLE(long long);
	YAML_DEFINE_CONVERT_STREAMABLE(unsigned long long);
	
	YAML_DEFINE_CONVERT_STREAMABLE(char);
	YAML_DEFINE_CONVERT_STREAMABLE(unsigned char);
	
	YAML_DEFINE_CONVERT_STREAMABLE(float);
	YAML_DEFINE_CONVERT_STREAMABLE(double);
	YAML_DEFINE_CONVERT_STREAMABLE(long double);
	
#undef YAML_DEFINE_CONVERT_STREAMABLE

	template<typename K, typename V>
	struct convert<std::map<K, V> > {
		static Value encode(const std::map<K, V>& rhs) {
			Value value(ValueType::Map);
			for(typename std::map<K, V>::const_iterator it=rhs.begin();it!=rhs.end();++it)
				value[it->first] = it->second;
			return value;
		}
		
		static bool decode(const Value& value, std::map<K, V>& rhs) {
			rhs.clear();
			return false;
		}
	};
}

#endif // VALUE_CONVERT_H_62B23520_7C8E_11DE_8A39_0800200C9A66
