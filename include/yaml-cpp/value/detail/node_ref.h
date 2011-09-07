#ifndef VALUE_DETAIL_NODE_REF_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_NODE_REF_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/type.h"
#include "yaml-cpp/value/ptr.h"
#include "yaml-cpp/value/detail/node_data.h"

namespace YAML
{
	namespace detail
	{
		class node_ref
		{
		public:
			node_ref(): m_pData(new node_data) {}
			
			ValueType::value type() const { return m_pData->type(); }
			
			void set_data(const node_ref& rhs) { m_pData = rhs.m_pData; }
			
			void set_type(ValueType::value type) { m_pData->set_type(type); }
			void set_null() { m_pData->set_null(); }
			void set_scalar(const std::string& scalar) { m_pData->set_scalar(scalar); }
			
			// indexing
			template<typename Key> shared_node get(const Key& key, shared_memory_holder pMemory) const { return static_cast<const node_data&>(*m_pData).get(key, pMemory); }
			template<typename Key> shared_node get(const Key& key, shared_memory_holder pMemory) { return m_pData->get(key, pMemory); }
			template<typename Key> bool remove(const Key& key, shared_memory_holder pMemory) { return m_pData->remove(key, pMemory); }
			
			shared_node get(shared_node pKey) const { return static_cast<const node_data&>(*m_pData).get(pKey); }
			shared_node get(shared_node pKey) { return m_pData->get(pKey); }
			bool remove(shared_node pKey) { return m_pData->remove(pKey); }
			
		private:
			shared_node_data m_pData;
		};
	}
}

#endif // VALUE_DETAIL_NODE_REF_H_62B23520_7C8E_11DE_8A39_0800200C9A66
