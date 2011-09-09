#ifndef VALUE_DETAIL_NODE_REF_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_NODE_REF_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/value/type.h"
#include "yaml-cpp/value/ptr.h"
#include "yaml-cpp/value/detail/node_data.h"
#include <boost/utility.hpp>

namespace YAML
{
	namespace detail
	{
		class node_ref: private boost::noncopyable
		{
		public:
			node_ref() {}
			
			ValueType::value type() const { return m_pData ? m_pData->type() : ValueType::Undefined; }
			const std::string& scalar() const { return m_pData ? m_pData->scalar() : node_data::empty_scalar; }
			
			void set_data(const node_ref& rhs) { m_pData = rhs.m_pData; }
			
			void set_type(ValueType::value type) { ensure_data_exists(); m_pData->set_type(type); }
			void set_null() { ensure_data_exists(); m_pData->set_null(); }
			void set_scalar(const std::string& scalar) { ensure_data_exists(); m_pData->set_scalar(scalar); }
			
			// size/iterator
			std::size_t size() const { return m_pData ? m_pData->size() : 0; }
			
			const_iterator begin(shared_memory_holder pMemory) const { return m_pData ? static_cast<const node_data&>(*m_pData).begin(pMemory) : const_iterator(); }
			iterator begin(shared_memory_holder pMemory) {return m_pData ? m_pData->begin(pMemory) : iterator(); }
			
			const_iterator end(shared_memory_holder pMemory) const { return m_pData ? static_cast<const node_data&>(*m_pData).end(pMemory) : const_iterator(); }
			iterator end(shared_memory_holder pMemory) {return m_pData ? m_pData->end(pMemory) : iterator(); }

			// sequence
			void append(node& node, shared_memory_holder pMemory) { ensure_data_exists(); m_pData->append(node, pMemory); }
			
			// indexing
			template<typename Key> node& get(const Key& key, shared_memory_holder pMemory) const { return m_pData ? static_cast<const node_data&>(*m_pData).get(key, pMemory) : pMemory->create_node(); }
			template<typename Key> node& get(const Key& key, shared_memory_holder pMemory) { ensure_data_exists(); return m_pData->get(key, pMemory); }
			template<typename Key> bool remove(const Key& key, shared_memory_holder pMemory) { return m_pData ? m_pData->remove(key, pMemory) : false; }
			
			node& get(node& key, shared_memory_holder pMemory) const { return m_pData ? static_cast<const node_data&>(*m_pData).get(key, pMemory) : pMemory->create_node(); }
			node& get(node& key, shared_memory_holder pMemory) { ensure_data_exists(); return m_pData->get(key, pMemory); }
			bool remove(node& key, shared_memory_holder pMemory) { return m_pData ? m_pData->remove(key, pMemory) : false; }
			
		private:
			void ensure_data_exists() { if(!m_pData) m_pData.reset(new node_data); }
			
		private:
			shared_node_data m_pData;
		};
	}
}

#endif // VALUE_DETAIL_NODE_REF_H_62B23520_7C8E_11DE_8A39_0800200C9A66
