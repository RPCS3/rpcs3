#ifndef VALUE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/value/detail/node.h"
#include "yaml-cpp/value/detail/node_data.h"

namespace YAML
{
	namespace detail
	{
		// indexing
		template<typename Key>
		inline shared_node node_data::get(const Key& key, shared_memory_holder pMemory) const
		{
			if(m_type != ValueType::Map)
				return shared_node(new node);
			
			for(node_map::const_iterator it=m_map.begin();it!=m_map.end();++it) {
				if(equals(it->first, key, pMemory))
					return it->second;
			}
			
			return shared_node(new node);
		}
		
		template<typename Key>
		inline shared_node node_data::get(const Key& key, shared_memory_holder pMemory)
		{
			// TODO: check if 'key' is index-like, and we're a sequence
			
			switch(m_type) {
				case ValueType::Undefined:
				case ValueType::Null:
				case ValueType::Scalar:
					m_type = ValueType::Map;
					m_map.clear();
					break;
				case ValueType::Sequence:
					convert_sequence_to_map();
					break;
				case ValueType::Map:
					break;
			}
			
			for(node_map::const_iterator it=m_map.begin();it!=m_map.end();++it) {
				if(equals(it->first, key, pMemory))
					return it->second;
			}
			
			shared_node pKey = convert_to_node(key, pMemory);
			shared_node pValue(new node);
			m_map.push_back(kv_pair(pKey, pValue));
			return pValue;
		}
		
		template<typename Key>
		inline bool node_data::remove(const Key& key, shared_memory_holder pMemory)
		{
			if(m_type != ValueType::Map)
				return false;
			
			for(node_map::iterator it=m_map.begin();it!=m_map.end();++it) {
				if(equals(it->first, key, pMemory)) {
					m_map.erase(it);
					return true;
				}
			}
			
			return false;
		}

		template<typename T>
		inline bool node_data::equals(detail::shared_node pNode, const T& rhs, detail::shared_memory_holder pMemory)
		{
			T lhs;
			if(convert(Value(pNode, pMemory), lhs))
				return lhs == rhs;
			return false;
		}
		
		template<typename T>
		inline shared_node node_data::convert_to_node(const T& rhs, detail::shared_memory_holder pMemory)
		{
			Value value = convert(rhs);
			pMemory->merge(*value.m_pMemory);
			return value.m_pNode;
		}
	}
}

#endif // VALUE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
