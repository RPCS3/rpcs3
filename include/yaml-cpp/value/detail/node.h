#ifndef VALUE_DETAIL_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/type.h"
#include "yaml-cpp/value/ptr.h"
#include "yaml-cpp/value/detail/node_ref.h"

namespace YAML
{
	namespace detail
	{
		class node
		{
		public:
			explicit node(shared_node_ref pRef): m_pRef(pRef) {}

			bool is(const node& rhs) const { return m_pRef == rhs.m_pRef; }
			ValueType::value type() const { return m_pRef->type(); }
			
			const std::string& scalar() const { return m_pRef->scalar(); }
			
			void set_ref(const node& rhs) { m_pRef = rhs.m_pRef; }
			void set_data(const node& rhs) { m_pRef->set_data(*rhs.m_pRef); }
				
			void set_type(ValueType::value type) { m_pRef->set_type(type); }
			void set_null() { m_pRef->set_null(); }
			void set_scalar(const std::string& scalar) { m_pRef->set_scalar(scalar); }

			// indexing
			template<typename Key> shared_node get(const Key& key, shared_memory_holder pMemory) const { return static_cast<const node_ref&>(*m_pRef).get(key, pMemory); }
			template<typename Key> shared_node get(const Key& key, shared_memory_holder pMemory) { return m_pRef->get(key, pMemory); }
			template<typename Key> bool remove(const Key& key, shared_memory_holder pMemory) { return m_pRef->remove(key, pMemory); }
			
			shared_node get(shared_node pKey, shared_memory_holder pMemory) const { return static_cast<const node_ref&>(*m_pRef).get(pKey, pMemory); }
			shared_node get(shared_node pKey, shared_memory_holder pMemory) { return m_pRef->get(pKey, pMemory); }
			bool remove(shared_node pKey, shared_memory_holder pMemory) { return m_pRef->remove(pKey, pMemory); }

		private:
			shared_node_ref m_pRef;
		};
	}
}

#endif // VALUE_DETAIL_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
