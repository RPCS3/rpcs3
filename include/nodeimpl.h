#pragma once

#ifndef NODEIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODEIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66


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
	inline void operator >> (const Node& node, T& value) {
		if(!node.Read(value))
			throw InvalidScalar(node.m_mark);
	}
	
	template <typename T>
	inline const Node *Node::FindValue(const T& key) const {
		if(!m_pContent)
			return 0;
		
		for(Iterator it=begin();it!=end();++it) {
			T t;
			if(it.first().Read(t)) {
				if(key == t)
					return &it.second();
			}
		}
		
		return 0;
	}
	
	inline const Node *Node::FindValue(const char *key) const {
		return FindValue(std::string(key));
	}
	
	template <typename T>
	inline const Node& Node::GetValue(const T& key) const {
		if(!m_pContent)
			throw BadDereference();
		
		for(Iterator it=begin();it!=end();++it) {
			T t;
			if(it.first().Read(t)) {
				if(key == t)
					return it.second();
			}
		}
		
		throw MakeTypedKeyNotFound(m_mark, key);
	}
	
	template <typename T>
	inline const Node& Node::operator [] (const T& key) const {
		return GetValue(key);
	}
	
	inline const Node& Node::operator [] (const char *key) const {
		return GetValue(std::string(key));
	}
}

#endif // NODEIMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
