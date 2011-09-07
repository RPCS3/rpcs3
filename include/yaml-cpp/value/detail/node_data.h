#ifndef VALUE_DETAIL_NODE_DATA_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_NODE_DATA_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/type.h"
#include "yaml-cpp/value/ptr.h"
#include <pair>
#include <vector>

namespace YAML
{
	namespace detail
	{
		class node_data
		{
		public:
			explicit node_data(const std::string& scalar);
			
			ValueType::value type() const { return m_type; }
			const std::string scalar() const { return m_data; }
			
		private:
			ValueType::value m_type;
			
			// scalar
			std::string m_scalar;
			
			// sequence
			typedef std::vector<shared_node> node_seq;
			node_seq m_sequence;
			
			// map
			typedef std::pair<shared_node, shared_node> kv_pair;
			typedef std::vector<kv_pair> node_map;
			node_map m_map;
		};
	}
}

#endif // VALUE_DETAIL_NODE_DATA_H_62B23520_7C8E_11DE_8A39_0800200C9A66
