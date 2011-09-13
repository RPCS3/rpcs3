#ifndef NODE_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODE_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/iterator.h"
#include "yaml-cpp/node/detail/memory.h"
#include "yaml-cpp/node/detail/node.h"
#include <string>

namespace YAML
{
	inline Node::Node(): m_pNode(0)
	{
	}
	
	inline Node::Node(NodeType::value type): m_pMemory(new detail::memory_holder), m_pNode(&m_pMemory->create_node())
	{
		m_pNode->set_type(type);
	}
	
	template<typename T>
	inline Node::Node(const T& rhs): m_pMemory(new detail::memory_holder), m_pNode(&m_pMemory->create_node())
	{
		Assign(rhs);
	}
	
	inline Node::Node(const Node& rhs): m_pMemory(rhs.m_pMemory), m_pNode(rhs.m_pNode)
	{
	}
	
	inline Node::Node(detail::node& node, detail::shared_memory_holder pMemory): m_pMemory(pMemory), m_pNode(&node)
	{
	}

	inline Node::~Node()
	{
	}

	inline void Node::EnsureNodeExists() const
	{
		if(!m_pNode) {
			m_pMemory.reset(new detail::memory_holder);
			m_pNode = &m_pMemory->create_node();
			m_pNode->set_null();
		}
	}

	inline NodeType::value Node::Type() const
	{
		return m_pNode ? m_pNode->type() : NodeType::Null;
	}
	
	// access
	template<typename T>
	inline const T Node::as() const
	{
		if(!m_pNode)
			throw std::runtime_error("Unable to convert to type");
			
		T t;
		if(convert<T>::decode(*this, t))
			return t;
		throw std::runtime_error("Unable to convert to type");
	}
	
	template<>
	inline const std::string Node::as() const
	{
		if(Type() != NodeType::Scalar)
			throw std::runtime_error("Unable to convert to string, not a scalar");
		return scalar();
	}

	inline const std::string& Node::scalar() const
	{
		return m_pNode ? m_pNode->scalar() : detail::node_data::empty_scalar;
	}

	// assignment
	inline bool Node::is(const Node& rhs) const
	{
		return m_pNode ? m_pNode->is(*rhs.m_pNode) : false;
	}

	template<typename T>
	inline Node& Node::operator=(const T& rhs)
	{
		Assign(rhs);
		return *this;
	}
	
	template<typename T>
	inline void Node::Assign(const T& rhs)
	{
		AssignData(convert<T>::encode(rhs));
	}

	template<>
	inline void Node::Assign(const std::string& rhs)
	{
		EnsureNodeExists();
		m_pNode->set_scalar(rhs);
	}

	inline void Node::Assign(const char *rhs)
	{
		EnsureNodeExists();
		m_pNode->set_scalar(rhs);
	}

	inline void Node::Assign(char *rhs)
	{
		EnsureNodeExists();
		m_pNode->set_scalar(rhs);
	}
	
	inline Node& Node::operator=(const Node& rhs)
	{
		if(is(rhs))
			return *this;
		AssignNode(rhs);
		return *this;
	}

	inline void Node::AssignData(const Node& rhs)
	{
		EnsureNodeExists();
		rhs.EnsureNodeExists();
		
		m_pNode->set_data(*rhs.m_pNode);
		m_pMemory->merge(*rhs.m_pMemory);
	}

	inline void Node::AssignNode(const Node& rhs)
	{
		rhs.EnsureNodeExists();

		if(!m_pNode) {
			m_pNode = rhs.m_pNode;
			m_pMemory = rhs.m_pMemory;
			return;
		}

		m_pNode->set_ref(*rhs.m_pNode);
		m_pMemory->merge(*rhs.m_pMemory);
		m_pNode = rhs.m_pNode;
	}

	// size/iterator
	inline std::size_t Node::size() const
	{
		return m_pNode ? m_pNode->size() : 0;
	}

	inline const_iterator Node::begin() const
	{
		return m_pNode ? const_iterator(m_pNode->begin(), m_pMemory) : const_iterator();
	}
	
	inline iterator Node::begin()
	{
		return m_pNode ? iterator(m_pNode->begin(), m_pMemory) : iterator();
	}

	inline const_iterator Node::end() const
	{
		return m_pNode ? const_iterator(m_pNode->end(), m_pMemory) : const_iterator();
	}

	inline iterator Node::end()
	{
		return m_pNode ? iterator(m_pNode->end(), m_pMemory) : iterator();
	}
	
	// sequence
	template<typename T>
	inline void Node::append(const T& rhs)
	{
		append(Node(rhs));
	}
	
	inline void Node::append(const Node& rhs)
	{
		EnsureNodeExists();
		rhs.EnsureNodeExists();
		
		m_pNode->append(*rhs.m_pNode, m_pMemory);
		m_pMemory->merge(*rhs.m_pMemory);
	}

	// indexing
	template<typename Key>
	inline const Node Node::operator[](const Key& key) const
	{
		EnsureNodeExists();
		detail::node& value = static_cast<const detail::node&>(*m_pNode).get(key, m_pMemory);
		return Node(value, m_pMemory);
	}
	
	template<typename Key>
	inline Node Node::operator[](const Key& key)
	{
		EnsureNodeExists();
		detail::node& value = m_pNode->get(key, m_pMemory);
		return Node(value, m_pMemory);
	}
	
	template<typename Key>
	inline bool Node::remove(const Key& key)
	{
		EnsureNodeExists();
		return m_pNode->remove(key, m_pMemory);
	}
	
	inline const Node Node::operator[](const Node& key) const
	{
		EnsureNodeExists();
		key.EnsureNodeExists();
		detail::node& value = static_cast<const detail::node&>(*m_pNode).get(*key.m_pNode, m_pMemory);
		return Node(value, m_pMemory);
	}
	
	inline Node Node::operator[](const Node& key)
	{
		EnsureNodeExists();
		key.EnsureNodeExists();
		detail::node& value = m_pNode->get(*key.m_pNode, m_pMemory);
		return Node(value, m_pMemory);
	}
	
	inline bool Node::remove(const Node& key)
	{
		EnsureNodeExists();
		key.EnsureNodeExists();
		return m_pNode->remove(*key.m_pNode, m_pMemory);
	}
	
	inline const Node Node::operator[](const char *key) const
	{
		return operator[](std::string(key));
	}
	
	inline Node Node::operator[](const char *key)
	{
		return operator[](std::string(key));
	}
	
	inline bool Node::remove(const char *key)
	{
		return remove(std::string(key));
	}
	
	inline const Node Node::operator[](char *key) const
	{
		return operator[](static_cast<const char *>(key));
	}
	
	inline Node Node::operator[](char *key)
	{
		return operator[](static_cast<const char *>(key));
	}
	
	inline bool Node::remove(char *key)
	{
		return remove(static_cast<const char *>(key));
	}

	// free functions

	inline bool operator==(const Node& lhs, const Node& rhs)
	{
		return lhs.is(rhs);
	}
}

#endif // NODE_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
