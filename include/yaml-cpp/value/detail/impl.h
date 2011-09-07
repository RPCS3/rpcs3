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
		inline shared_node node_data::operator[](const Key& key) const
		{
			if(m_type != ValueType::Map)
				return shared_node(new node);
			
			for(node_map::const_iterator it=m_map.begin();it!=m_map.end();++it) {
				if(it->first->equals(key))
					return it->second;
			}
			
			return shared_node(new node);
		}
		
		template<typename Key>
		inline shared_node node_data::operator[](const Key& key)
		{
		}
		
		template<typename Key>
		inline bool node_data::remove(const Key& key)
		{
		}
	}
}

#endif // VALUE_DETAIL_IMPL_H_62B23520_7C8E_11DE_8A39_0800200C9A66
