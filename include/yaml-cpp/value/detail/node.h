#ifndef VALUE_DETAIL_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

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
		class node
		{
		public:
			node(): m_pData(new node_data) {}

			ValueType::value type() const { return m_pData->type(); }
			
			void set_data(const node& rhs) { m_pData = rhs.m_pData; }
				
			void set_type(ValueType::value type) { m_pData->set_type(type); }
			void set_null() { m_pData->set_null(); }
			void set_scalar(const std::string& scalar) { m_pData->set_scalar(scalar); }
			
			template<typename T> bool equals(const T& rhs) const;
			
			// indexing
			template<typename Key> shared_node operator[](const Key& key) const { return (static_cast<const node_data&>(*m_pData))[key]; }
			template<typename Key> shared_node operator[](const Key& key) { return (*m_pData)[key]; }
			template<typename Key> bool remove(const Key& key) { return m_pData->remove(key); }
			
			shared_node operator[](shared_node pKey) const { return (static_cast<const node_data&>(*m_pData))[pKey]; }
			shared_node operator[](shared_node pKey) { return (*m_pData)[pKey]; }
			bool remove(shared_node pKey) { return m_pData->remove(pKey); }

		private:
			shared_node_data m_pData;
		};

		template<typename T>
		inline bool node::equals(const T& rhs) const
		{
		}
	}
}

#endif // VALUE_DETAIL_NODE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
