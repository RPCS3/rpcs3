#ifndef VALUE_VALUE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_VALUE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/ptr.h"
#include "yaml-cpp/value/type.h"
#include "yaml-cpp/value/detail/iterator_fwd.h"
#include <stdexcept>

namespace YAML
{
	class Value
	{
	public:
		friend class detail::node_data;
		
		Value();
		explicit Value(ValueType::value type);
		template<typename T> explicit Value(const T& rhs);
		Value(const Value& rhs);
		~Value();
		
		ValueType::value Type() const;
		
		// access
		template<typename T> const T as() const;
		const std::string& scalar() const;
		
		// assignment
		bool is(const Value& rhs) const;
		template<typename T> Value& operator=(const T& rhs);
		Value& operator=(const Value& rhs);
		
		// size/iterator
		std::size_t size() const;

		const_iterator begin() const;
		iterator begin();
		
		const_iterator end() const;
		iterator end();
		
		// sequence
		template<typename T> void append(const T& rhs);
		void append(const Value& rhs);
		
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
		
	private:
		explicit Value(detail::node& node, detail::shared_memory_holder pMemory);
		
		template<typename T> void Assign(const T& rhs);
		void Assign(const char *rhs);
		void Assign(char *rhs);

		void AssignData(const Value& rhs);
		void AssignNode(const Value& rhs);
		
	private:
		detail::shared_memory_holder m_pMemory;
		detail::node *m_pNode;
	};
	
	int compare(const Value& lhs, const Value& rhs);
	bool operator<(const Value& lhs, const Value& rhs);
	
	bool is(const Value& lhs, const Value& rhs);
	
	template<typename T>
	struct convert;
}

#endif // VALUE_VALUE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
