#ifndef VALUE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/node_ptr.h"

namespace YAML
{
	struct ValueType { enum value { Undefined, Null, Scalar, Sequence, Map }; };
	
	class Value
	{
	public:
		Value();
		explicit Value(ValueType::value type);
		~Value();
		
		ValueType::value Type() const;
		
		// access
		template<typename T> const T as() const;
		
		// assignment
		template<typename T> Value& operator=(const T& rhs) const;
		Value& operator=(const Value& rhs);
		
		// size/iterator
		std::size_t size() const;

		const_iterator begin() const;
		iterator begin();
		
		const_iterator end() const;
		iterator end();
		
		// indexing
		template<typename Key> const Value operator[](const Key& key) const;
		template<typename Key> Value operator[](const Key& key);
		template<typename Key> bool remove(const Key& key);

		const Value operator[](const Value& key) const;
		Value operator[](const Value& key);
		bool remove(const Value& key);
		
		const Value operator[](const char *key) const;
		Value operator[](const char *key);
		bool remove(const char *key);

		const Value operator[](char *key) const;
		Value operator[](char *key);
		bool remove(char *key);
	};
	
	int compare(const Value& lhs, const Value& rhs);
	bool operator<(const Value& lhs, const Value& rhs);
	
	bool is(const Value& lhs, const Value& rhs);
}

#endif // VALUE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
