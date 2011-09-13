#ifndef NODE_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODE_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/node/ptr.h"
#include "yaml-cpp/node/type.h"
#include "yaml-cpp/node/detail/iterator_fwd.h"
#include <stdexcept>

namespace YAML
{
	class Node
	{
	public:
		friend class NodeBuilder;
		friend class NodeEvents;
		friend class detail::node_data;
		template<typename, typename, typename> friend class detail::iterator_base;
		
		Node();
		explicit Node(NodeType::value type);
		template<typename T> explicit Node(const T& rhs);
		Node(const Node& rhs);
		~Node();
		
		NodeType::value Type() const;
		bool IsNull() const { return Type() == NodeType::Null; }
		bool IsScalar() const { return Type() == NodeType::Scalar; }
		bool IsSequence() const { return Type() == NodeType::Sequence; }
		bool IsMap() const { return Type() == NodeType::Map; }
		
		// access
		template<typename T> const T as() const;
		const std::string& Scalar() const;
		const std::string& Tag() const;
		void SetTag(const std::string& tag);

		// assignment
		bool is(const Node& rhs) const;
		template<typename T> Node& operator=(const T& rhs);
		Node& operator=(const Node& rhs);

		// size/iterator
		std::size_t size() const;

		const_iterator begin() const;
		iterator begin();
		
		const_iterator end() const;
		iterator end();
		
		// sequence
		template<typename T> void append(const T& rhs);
		void append(const Node& rhs);
		
		// indexing
		template<typename Key> const Node operator[](const Key& key) const;
		template<typename Key> Node operator[](const Key& key);
		template<typename Key> bool remove(const Key& key);

		const Node operator[](const Node& key) const;
		Node operator[](const Node& key);
		bool remove(const Node& key);
		
		const Node operator[](const char *key) const;
		Node operator[](const char *key);
		bool remove(const char *key);

		const Node operator[](char *key) const;
		Node operator[](char *key);
		bool remove(char *key);
		
	private:
		explicit Node(detail::node& node, detail::shared_memory_holder pMemory);
		
		void EnsureNodeExists() const;
		
		template<typename T> void Assign(const T& rhs);
		void Assign(const char *rhs);
		void Assign(char *rhs);

		void AssignData(const Node& rhs);
		void AssignNode(const Node& rhs);
		
	private:
		mutable detail::shared_memory_holder m_pMemory;
		mutable detail::node *m_pNode;
	};

	bool operator==(const Node& lhs, const Node& rhs);
	
	template<typename T>
	struct convert;
}

#endif // NODE_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
