#ifndef NODE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NODE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/node/detail/node.h"
#include "yaml-cpp/node/detail/node_data.h"

namespace YAML
{
	namespace detail
	{
		// indexing
		template<typename Key>
		inline node& node_data::get(const Key& key, shared_memory_holder pMemory) const
		{
			if(m_type != NodeType::Map)
				return pMemory->create_node();
			
			for(node_map::const_iterator it=m_map.begin();it!=m_map.end();++it) {
				if(equals(*it->first, key, pMemory))
					return *it->second;
			}
			
			return pMemory->create_node();
		}
		
		template<typename Key>
		inline node& node_data::get(const Key& key, shared_memory_holder pMemory)
		{
			// TODO: check if 'key' is index-like, and we're a sequence
			
			switch(m_type) {
				case NodeType::Undefined:
				case NodeType::Null:
				case NodeType::Scalar:
					m_type = NodeType::Map;
					m_map.clear();
					break;
				case NodeType::Sequence:
					convert_sequence_to_map(pMemory);
					break;
				case NodeType::Map:
					break;
			}
			
			for(node_map::const_iterator it=m_map.begin();it!=m_map.end();++it) {
				if(equals(*it->first, key, pMemory))
					return *it->second;
			}
			
			node& k = convert_to_node(key, pMemory);
			node& v = pMemory->create_node();
			m_map[&k] = &v;
			return v;
		}
		
		template<typename Key>
		inline bool node_data::remove(const Key& key, shared_memory_holder pMemory)
		{
			if(m_type != NodeType::Map)
				return false;
			
			for(node_map::iterator it=m_map.begin();it!=m_map.end();++it) {
				if(equals(*it->first, key, pMemory)) {
					m_map.erase(it);
					return true;
				}
			}
			
			return false;
		}

		template<typename T>
		inline bool node_data::equals(node& node, const T& rhs, shared_memory_holder pMemory)
		{
			T lhs;
			if(convert<T>::decode(Node(node, pMemory), lhs))
				return lhs == rhs;
			return false;
		}
		
		template<typename T>
		inline node& node_data::convert_to_node(const T& rhs, shared_memory_holder pMemory)
		{
			Node value = convert<T>::encode(rhs);
			pMemory->merge(*value.m_pMemory);
			return *value.m_pNode;
		}
	}
}

#endif // NODE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
