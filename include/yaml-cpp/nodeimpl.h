#ifndef NODEIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODEIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/nodeutil.h"
#include <cassert>

namespace YAML
{
	// implementation of templated things
	template <typename T>
	inline const T Node::to() const {
		T value;
		*this >> value;
		return value;
	}

	template <typename T>
	inline typename enable_if<is_scalar_convertible<T> >::type operator >> (const Node& node, T& value) {
		if(!ConvertScalar(node, value))
			throw InvalidScalar(node.m_mark);
	}
	
	template <typename T>
	inline const Node *Node::FindValue(const T& key) const {
		switch(m_type) {
			case NodeType::Null:
			case NodeType::Scalar:
				throw BadDereference();
			case NodeType::Sequence:
				return FindFromNodeAtIndex(*this, key);
			case NodeType::Map:
				return FindValueForKey(key);
		}
		assert(false);
		throw BadDereference();
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
		if(const Node *pValue = FindValue(key))
			return *pValue;
		throw MakeTypedKeyNotFound(m_mark, key);
	}
	
	template <typename T>
	inline const Node& Node::operator [] (const T& key) const {
		return GetValue(key);
	}
	
	inline const Node *Node::FindValue(const char *key) const {
		return FindValue(std::string(key));
	}

	inline const Node *Node::FindValue(char *key) const {
		return FindValue(std::string(key));
	}
	
	inline const Node& Node::operator [] (const char *key) const {
		return GetValue(std::string(key));
	}

	inline const Node& Node::operator [] (char *key) const {
		return GetValue(std::string(key));
	}
}

#endif // NODEIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
