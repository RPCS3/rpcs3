#ifndef VALUE_DETAIL_NODE_DATA_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_NODE_DATA_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/ptr.h"
#include "yaml-cpp/value/type.h"
#include <list>
#include <utility>
#include <vector>

namespace YAML
{
	namespace detail
	{
		class node_data
		{
		public:
			node_data();
			
			void set_type(ValueType::value type);
			void set_null();
			void set_scalar(const std::string& scalar);
			
			ValueType::value type() const { return m_isDefined ? m_type : ValueType::Undefined; }
			const std::string scalar() const { return m_scalar; }
			
			// indexing
			template<typename Key> shared_node operator[](const Key& key) const;
			template<typename Key> shared_node operator[](const Key& key);
			template<typename Key> bool remove(const Key& key);
			
			shared_node operator[](shared_node pKey) const;
			shared_node operator[](shared_node pKey);
			bool remove(shared_node pKey);
			
		private:
			void convert_sequence_to_map();

		private:
			bool m_isDefined;
			ValueType::value m_type;
			
			// scalar
			std::string m_scalar;
			
			// sequence
			typedef std::vector<shared_node> node_seq;
			node_seq m_sequence;
			
			// map
			typedef std::pair<shared_node, shared_node> kv_pair;
			typedef std::list<kv_pair> node_map;
			node_map m_map;
		};
	}
}

#endif // VALUE_DETAIL_NODE_DATA_H_62B23520_7C8E_11DE_8A39_0800200C9A66
