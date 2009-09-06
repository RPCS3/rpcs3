#pragma once

#ifndef NODEIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODEIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "nodeutil.h"

namespace YAML
{
	// implementation of templated things
	template <typename T>
	inline bool Node::Read(T& value) const {
		std::string scalar;
		if(!GetScalar(scalar))
			return false;
		
		return Convert(scalar, value);
	}
	
	template <typename T>
	inline const T Node::Read() const {
		T value;
		*this >> value;
		return value;
	}
	
	template <typename T>
	Node::operator T() const {
		return Read<T>();
	}

	template <typename T>
	inline void operator >> (const Node& node, T& value) {
		if(!node.Read(value))
			throw InvalidScalar(node.m_mark);
	}
	
	template <typename T>
	inline const Node *Node::FindValue(const T& key) const {
		switch(GetType()) {
			case CT_MAP:
				return FindValueForKey(key);
			case CT_SEQUENCE:
				return FindFromNodeAtIndex(*this, key);
			default:
				return 0;
		}
	}
	
	template <typename T>
	inline const Node *Node::FindValueForKey(const T& key) const {
		for(Iterator it=begin();it!=end();++it) {
			T t;
			if(it.first().Read(t)) {
				if(key == t)
					return &it.second();
			}
		}
		
		return 0;
	}
	
	template <typename T>
	inline const Node& Node::GetValue(const T& key) const {
		if(!m_pContent)
			throw BadDereference();
		
		const Node *pValue = FindValue(key);
		if(!pValue)
			throw MakeTypedKeyNotFound(m_mark, key);

		return *pValue;
	}
	
	template <typename T>
	inline const Node& Node::operator [] (const T& key) const {
		return GetValue(key);
	}
	
	inline const Node *Node::FindValue(const char *key) const {
		return FindValue(std::string(key));
	}
	
	inline const Node& Node::operator [] (const char *key) const {
		return GetValue(std::string(key));
	}

	template <typename T>
	inline bool operator == (const T& value, const Node& node) {
		return value == node.Read<T>();
	}
	
	template <typename T>
	inline bool operator == (const Node& node, const T& value) {
		return value == node.Read<T>();
	}
	
	template <typename T>
	inline bool operator != (const T& value, const Node& node) {
		return value != node.Read<T>();
	}
	
	template <typename T>
	inline bool operator != (const Node& node, const T& value) {
		return value != node.Read<T>();
	}

	inline bool operator == (const char *value, const Node& node) {
		return std::string(value) == node;
	}
	
	inline bool operator == (const Node& node, const char *value) {
		return std::string(value) == node;
	}
	
	inline bool operator != (const char *value, const Node& node) {
		return std::string(value) != node;
	}
	
	inline bool operator != (const Node& node, const char *value) {
		return std::string(value) != node;
	}
	
}

#endif // NODEIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
