#ifndef VALUE_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/value/value.h"
#include "yaml-cpp/value/detail/memory.h"
#include <string>

namespace YAML
{
	inline Value::Value(): m_pMemory(new detail::memory_holder)
	{
	}
	
	template<typename T>
	inline Value::Value(const T& rhs): m_pMemory(new detail::memory_holder)
	{
		Assign(rhs);
	}
	
	inline Value::Value(const Value& rhs): m_pNode(rhs.m_pNode), m_pMemory(rhs.m_pMemory)
	{
	}
	
	inline Value::~Value()
	{
	}

	// access
	template<typename T>
	inline const T Value::as() const
	{
		T t;
		if(convert<T>(*this, t))
			return t;
		throw std::runtime_error("Unable to convert to type");
	}
	
	// assignment
	template<typename T>
	inline Value& Value::operator=(const T& rhs)
	{
		Assign(rhs);
		return *this;
	}
	
	template<typename T>
	inline void Value::Assign(const T& rhs)
	{
		AssignData(convert<T>(rhs));
	}

	template<>
	inline void Value::Assign(const std::string& rhs)
	{
		EnsureNodeExists();
		m_pNode->set_scalar(rhs);
	}

	inline void Value::Assign(const char *rhs)
	{
		EnsureNodeExists();
		m_pNode->set_scalar(rhs);
	}

	inline void Value::Assign(char *rhs)
	{
		EnsureNodeExists();
		m_pNode->set_scalar(rhs);
	}
	
	inline Value& Value::operator=(const Value& rhs)
	{
		if(is(*this, rhs))
			return *this;
		AssignNode(rhs);
		return *this;
	}
	
	void Value::EnsureNodeExists()
	{
		if(!m_pNode)
			m_pNode = m_pMemory->create_node();
	}
	
	void Value::AssignData(const Value& rhs)
	{
		EnsureNodeExists();
		if(!rhs.m_pNode)
			throw std::runtime_error("Tried to assign an undefined value");
		
		m_pNode->assign_data(*rhs.m_pNode);
		m_pMemory->merge(*rhs.m_pMemory);
	}

	void Value::AssignNode(const Value& rhs)
	{
		m_pNode = rhs.m_pNode;
		m_pMemory->merge(*rhs.m_pMemory);
	}

	// size/iterator
	inline std::size_t Value::size() const
	{
		return 0;
	}

	inline const_iterator Value::begin() const
	{
		return const_iterator();
	}
	
	inline iterator Value::begin()
	{
		return iterator();
	}

	inline const_iterator Value::end() const
	{
		return const_iterator();
	}

	inline iterator Value::end()
	{
		return iterator();
	}
	
	// indexing
	template<typename Key>
	inline const Value Value::operator[](const Key& key) const
	{
		return Value();
	}
	
	template<typename Key>
	inline Value Value::operator[](const Key& key)
	{
		return Value();
	}
	
	template<typename Key>
	inline bool Value::remove(const Key& key)
	{
		return false;
	}
	
	inline const Value Value::operator[](const Value& key) const
	{
		return Value();
	}
	
	inline Value Value::operator[](const Value& key)
	{
		return Value();
	}
	
	inline bool Value::remove(const Value& key)
	{
		return false;
	}
	
	inline const Value Value::operator[](const char *key) const
	{
		return Value();
	}
	
	inline Value Value::operator[](const char *key)
	{
		return Value();
	}
	
	inline bool Value::remove(const char *key)
	{
		return false;
	}
	
	inline const Value Value::operator[](char *key) const
	{
		return Value();
	}
	
	inline Value Value::operator[](char *key)
	{
		return Value();
	}
	
	inline bool Value::remove(char *key)
	{
		return false;
	}

	// free functions
	inline int compare(const Value& lhs, const Value& rhs)
	{
		return 0;
	}
	
	inline bool operator<(const Value& lhs, const Value& rhs)
	{
		return false;
	}
	
	inline bool is(const Value& lhs, const Value& rhs)
	{
		return false;
	}
}

#endif // VALUE_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
